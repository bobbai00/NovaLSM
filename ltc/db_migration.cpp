
//
// Created by Haoyu Huang on 6/18/20.
// Copyright (c) 2020 University of Southern California. All rights reserved.
//

#include "db_migration.h"

#include "db_helper.h"
#include "db/db_impl.h"
#include "log/log_recovery.h"
#include "ltc/compaction_thread.h"

namespace nova {
    DBMigration::DBMigration(leveldb::MemManager *mem_manager,
                             leveldb::StoCBlockClient *client,
                             nova::StoCInMemoryLogFileManager *log_manager,
                             leveldb::StocPersistentFileManager *stoc_file_manager,
                             const std::vector<RDMAMsgHandler *> &bg_rdma_msg_handlers,
                             const std::vector<leveldb::EnvBGThread *> &bg_compaction_threads,
                             const std::vector<leveldb::EnvBGThread *> &bg_flush_memtable_threads)
            :
            mem_manager_(mem_manager),
            client_(client),
            log_manager_(log_manager),
            stoc_file_manager_(stoc_file_manager),
            bg_rdma_msg_handlers_(bg_rdma_msg_handlers),
            bg_compaction_threads_(bg_compaction_threads),
            bg_flush_memtable_threads_(bg_flush_memtable_threads) {
        sem_init(&sem_, 0, 0);
    }

    void DBMigration::AddSourceMigrateDB(const std::vector<nova::LTCFragment *> &frags) {
        mu.lock();
        for (auto frag: frags) {
            DBMeta meta = {};
            meta.migrate_type = MigrateType::SOURCE;
            meta.source_fragment = frag;
            db_metas.push_back(meta);
        }
        mu.unlock();
        sem_post(&sem_);
    }

    void DBMigration::AddDestMigrateDB(char *buf, uint32_t msg_size) {
        mu.lock();
        DBMeta meta = {};
        meta.migrate_type = MigrateType::DESTINATION;
        meta.source_fragment = nullptr;
        meta.buf = buf;
        meta.msg_size = msg_size;
        db_metas.push_back(meta);
        mu.unlock();
        sem_post(&sem_);
    }

    void DBMigration::Start() {
        while (true) {
            sem_wait(&sem_);

            mu.lock();
            std::vector<DBMeta> rdbs = db_metas;
            db_metas.clear();
            mu.unlock();
            uint32_t cfg_id = nova::NovaConfig::config->current_cfg_id;

            NOVA_ASSERT(cfg_id == 1);

            std::vector<nova::LTCFragment *> source_migrates;
            std::vector<DBMeta> dest_migrates;

            for (auto dbmeta : rdbs) {
                if (dbmeta.migrate_type == MigrateType::SOURCE) {
                    source_migrates.push_back(dbmeta.source_fragment);
                } else {
                    dest_migrates.push_back(dbmeta);
                }
            }

            if (!source_migrates.empty()) {
                MigrateDB(source_migrates);
            }
            for (auto dbmeta : dest_migrates) {
                RecoverDBMeta(dbmeta, cfg_id);
            }
        }
    }

    void DBMigration::MigrateDB(const std::vector<nova::LTCFragment *> &migrate_frags) {
        // bump up the configuration id.
        std::vector<char *> bufs;
        std::vector<uint32_t> msg_sizes;
        uint32_t scid = mem_manager_->slabclassid(0,
                                                  NovaConfig::config->max_stoc_file_size);
        for (auto frag : migrate_frags) {
            leveldb::DBImpl *db = reinterpret_cast<leveldb::DBImpl *>(frag->db);
            char *buf = mem_manager_->ItemAlloc(0, scid);
            msg_sizes.push_back(db->EncodeDBMetadata(buf, log_manager_));
            db->StopCompaction();
            bufs.push_back(buf);
        }

        // Inform the destination of the database metadata.
        for (int i = 0; i < migrate_frags.size(); i++) {
            client_->InitiateRDMAWRITE(migrate_frags[i]->ltc_server_id, bufs[i],
                                       msg_sizes[i]);
        }
        for (int i = 0; i < migrate_frags.size(); i++) {
            client_->Wait();
        }
        for (int i = 0; i < migrate_frags.size(); i++) {
            mem_manager_->FreeItem(0, bufs[i], scid);
        }
    }

    void
    DBMigration::RecoverDBMeta(DBMeta dbmeta, int cfg_id) {
        // Open the new database.
        NOVA_ASSERT(dbmeta.buf);
        NOVA_ASSERT(dbmeta.buf[0] == leveldb::StoCRequestType::LTC_MIGRATION);
        char *charbuf = dbmeta.buf;
        charbuf += 1;
        leveldb::Slice buf(charbuf, nova::NovaConfig::config->max_stoc_file_size);

        uint32_t dbindex = 0;
        uint32_t version_id = 0;
        uint64_t last_sequence = 0;
        uint64_t next_file_number = 0;
        uint64_t memtable_id_seq = 0;
        NOVA_ASSERT(DecodeFixed32(&buf, &dbindex));
        NOVA_ASSERT(DecodeFixed32(&buf, &version_id));
        NOVA_ASSERT(DecodeFixed64(&buf, &last_sequence));
        NOVA_ASSERT(DecodeFixed64(&buf, &next_file_number));
        NOVA_ASSERT(DecodeFixed64(&buf, &memtable_id_seq));

        NOVA_LOG(rdmaio::INFO)
            << fmt::format("{} {} {} {} {}", dbindex, version_id, last_sequence, next_file_number, memtable_id_seq);
        auto reorg = new leveldb::LTCCompactionThread(mem_manager_);
        auto coord = new leveldb::LTCCompactionThread(mem_manager_);
        auto client = new leveldb::StoCBlockClient(dbindex, stoc_file_manager_);
        auto db = CreateDatabase(cfg_id, dbindex, nullptr, nullptr, mem_manager_, client, bg_compaction_threads_,
                                 bg_flush_memtable_threads_, reorg, coord);
        coord->db_ = db;
        coord->stoc_client_ = new leveldb::StoCBlockClient(dbindex, stoc_file_manager_);
        coord->stoc_client_->rdma_msg_handlers_ = bg_rdma_msg_handlers_;
        coord->thread_id_ = dbindex;
        auto frag = nova::NovaConfig::config->cfgs[cfg_id]->fragments[dbindex];
        frag->db = db;
        auto dbimpl = reinterpret_cast<leveldb::DBImpl *>(db);
        dbimpl->log_manager_ = log_manager_;
        {
            auto c = reinterpret_cast<leveldb::StoCBlockClient *>(dbimpl->options_.stoc_client);
            c->rdma_msg_handlers_ = bg_rdma_msg_handlers_;
        }

        db->processed_writes_ = 0;
        db->number_of_puts_no_wait_ = 0;
        db->number_of_puts_wait_ = 0;
        db->number_of_steals_ = 0;
        db->number_of_wait_due_to_contention_ = 0;
        db->number_of_gets_ = 0;
        db->number_of_memtable_hits_ = 0;

        std::unordered_map<uint32_t, leveldb::MemTableLogFilePair> memtables_to_recover;
        // bump up the numbers to avoid conflicts.
        last_sequence += 100000;
        next_file_number += 100000;
        memtable_id_seq += 10;
        dbimpl->RecoverDBMetadata(buf, version_id, last_sequence, next_file_number, memtable_id_seq, log_manager_,
                                  &memtables_to_recover);
        std::unordered_map<uint32_t, leveldb::MemTableLogFilePair> actual_memtables_to_recover;
        for (const auto &memtable : memtables_to_recover) {
            if (memtable.second.server_logbuf.empty() || !memtable.second.memtable) {
                NOVA_LOG(rdmaio::INFO) << fmt::format("Nothing to recover for memtable {}-{}", dbindex, memtable.first);
                if (memtable.second.memtable) {
                    memtable.second.memtable->SetReadyToProcessRequests();
                }
                continue;
            }
            actual_memtables_to_recover[memtable.first] = memtable.second;
        }
        // The database is ready to process requests immediately.
        if (nova::NovaConfig::config->ltc_migration_policy == LTCMigrationPolicy::IMMEDIATE) {
            frag->is_ready_ = true;
            frag->is_ready_signal_.SignalAll();
        }
        leveldb::LogRecovery recover(mem_manager_, client_);
        recover.Recover(actual_memtables_to_recover, cfg_id, dbindex);
        threads_for_new_dbs_.emplace_back(&leveldb::LTCCompactionThread::Start, reorg);
        threads_for_new_dbs_.emplace_back(&leveldb::LTCCompactionThread::Start, coord);
        db->StartCompaction();

        frag->is_ready_ = true;
        frag->is_complete_ = true;
        frag->is_ready_signal_.SignalAll();

        uint32_t scid = mem_manager_->slabclassid(0, dbmeta.msg_size);
        mem_manager_->FreeItem(0, dbmeta.buf, scid);
    }
}