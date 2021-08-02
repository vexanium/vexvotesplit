#include "vexvotesplit.hpp"

namespace vexvote {
    void vexvotesplit::scanvote(const name &voter, const uint64_t &createat, const uint64_t &voteid, const uint64_t &totalvotetime)
    {
        auto producer = _producer.get_or_default();
        eosio_assert(producer.stopStatu == 0, "In stopStatu state!");
        
        eosio_assert(producer.checkfromid == voteid, "front id not finish yet!");
        require_auth(_self);

        if((voteid == 0) || (totalvotetime-1 != producer.maxcheckid))
        {
            producer.maxcheckid = totalvotetime - 1;
            _producer.set(producer, _self);

            updatebp(totalvotetime);
        }
        
        bool bnvoters = false;
        if((producer.scanStatu == 1) && (createat <= producer.lastvotetime))
        {
            bnvoters = true;
            producer.checkfromid = voteid + 1;
            if(producer.checkfromid > producer.maxcheckid)
            {
                producer.checkfromid = 0;
                producer.scanStatu = 0;
            }
            _producer.set(producer, _self);
        }
        else if((producer.scanStatu == 0) && (createat > producer.highvotetime))
        {
            bnvoters = true;
            producer.checkfromid = voteid + 1;
            producer.highvotetime = createat;
            _producer.set(producer, _self);
        }
        else if((producer.scanStatu == 0) && (createat <= producer.lastvotetime))
        {   
            producer.lastvotetime = producer.highvotetime;
            producer.checkfromid = 0;

            if(producer.lastdistramount > 0)
            {
                producer.checkStatu = 1;
                producer.maxcheckid = producer.maxvoteid;
            }
            _producer.set(producer, _self);
            
        }
         _producer.set(producer, _self);

        if(bnvoters)
        {
            struct voter_info {
                name                owner;     /// the voter
                name                proxy;     /// the proxy set by the voter, if any
                std::vector<name>   producers; /// the producers approved by this voter if no proxy set
                int64_t             staked = 0;
                double              last_vote_weight = 0; /// the vote weight cast the last time the vote was updated
                double              proxied_vote_weight= 0; /// the total vote weight delegated to this voter as a proxy
                bool                is_proxy = 0; /// whether the voter is a proxy for others


                uint32_t            flags1 = 0;
                uint32_t            reserved2 = 0;
                eosio::asset        reserved3;

                uint64_t primary_key()const { return owner.value; }

                enum class flags1_fields : uint32_t {
                    ram_managed = 1,
                    net_managed = 2,
                    cpu_managed = 4
                };
            };
            typedef eosio::multi_index< "voters"_n, voter_info >  voters_table;
            voters_table voters(name("vexcore"), name("vexcore").value);

            auto itrvote = voters.find(voter.value);
            eosio_assert(itrvote != voters.end(), "voter not exist in voters table!");
            auto producers = itrvote->producers;
            bool voted = false;
            for(uint8_t i= 0; i<= producers.size()-1; i++)
            {
                if(producers[i] == producer.producer)
                {
                    voted = true;
                    break;
                }
            }
            
            auto indexvoter = _voters.get_index<"voter"_n>();
            auto itr = indexvoter.lower_bound(voter.value);       
            if (itr->voter.value != voter.value)
            {
                if(voted) 
                {
                    auto newid  = _voters.available_primary_key();
                    double weight = itrvote->last_vote_weight/producers.size();

                    producer.maxvoteid = newid;     
                    producer.calc_total_votes += weight;    
                    _producer.set(producer, _self);

                    _voters.emplace( _self, [&]( auto& u ) {
                        u.id = newid;
                        u.voter = voter;
                        u.voteweight = weight;  
                        u.votetime = createat;   
                    });
                }
            }
            else
            {
                auto  itrmd = _voters.find(itr->id);
                if(voted)
                {
                    double weight = itrvote->last_vote_weight/producers.size();

                    producer.calc_total_votes -= itrmd->voteweight; 
                    producer.calc_total_votes += weight;  
                    _producer.set(producer, _self);

                    _voters.modify(itrmd, _self, [&](auto &u) {                 
                        u.voteweight = weight;  
                        u.votetime = createat; 
                    });  
                }
                else
                {
                    producer.calc_total_votes -= itrmd->voteweight; 
                    _producer.set(producer, _self);

                    _voters.erase(itrmd);
                }
            }           
        }
        _producer.set(producer, _self);
    }

    void vexvotesplit::checkvoters(const uint64_t &id)
    {
        auto producer = _producer.get_or_default();
        eosio_assert(producer.stopStatu == 0, "In stopStatu state!");
        eosio_assert(producer.scanStatu == 0 && producer.checkStatu == 1, "wrong check state!");
        eosio_assert(producer.checkfromid == id, "front id not finish yet!");

        require_auth(_self);

        asset distr_asset = asset(producer.lastdistramount, VEX_SYMBOL);

        if(id == 0)
        {
            updatebp(producer.maxcheckid + 1);

            asset poolasset = (*pet).get_balance(name(VEXIOTOKEN), _self, VEX_SYMBOL.code());
            eosio_assert(poolasset >= distr_asset, "have not enough VEX for distr!");
        }

        auto itr = _voters.find(id);
        eosio_assert(itr != _voters.end(), "id not found!");

        asset voter_distr = distr_asset;
        {
            //读取vote信息
            struct voter_info {
                name                owner;     /// the voter
                name                proxy;     /// the proxy set by the voter, if any
                std::vector<name>   producers; /// the producers approved by this voter if no proxy set
                int64_t             staked = 0;
                double              last_vote_weight = 0; /// the vote weight cast the last time the vote was updated
                double              proxied_vote_weight= 0; /// the total vote weight delegated to this voter as a proxy
                bool                is_proxy = 0; /// whether the voter is a proxy for others


                uint32_t            flags1 = 0;
                uint32_t            reserved2 = 0;
                eosio::asset        reserved3;

                uint64_t primary_key()const { return owner.value; }

                enum class flags1_fields : uint32_t {
                    ram_managed = 1,
                    net_managed = 2,
                    cpu_managed = 4
                };
            };
            typedef eosio::multi_index< "voters"_n, voter_info >  voters_table;
            voters_table voters(name("vexcore"), name("vexcore").value);

            auto itrvote = voters.find(itr->voter.value);
            eosio_assert(itrvote != voters.end(), "voter not exist in voters table!");
            auto producers = itrvote->producers;
            bool voted = false;
            for(uint8_t i= 0; i<= producers.size()-1; i++)
            {
                if(producers[i] == producer.producer)
                {
                    voted = true;
                    break;
                }
            }
            
            auto indexvoter = _voters.get_index<"voter"_n>();
            auto itrvtr = indexvoter.lower_bound(itr->voter.value);   
            {
                auto  itrmd = _voters.find(itrvtr->id);
                if(voted)
                {
                    double weight = itrvote->last_vote_weight/producers.size();
                    voter_distr.amount *= (weight / producer.total_votes);

                    producer.calc_total_votes -= itrmd->voteweight; 
                    producer.calc_total_votes += weight;  
                    _producer.set(producer, _self);

                    _voters.modify(itrmd, _self, [&](auto &u) {                 
                        u.voteweight = weight;
                        u.last_reward = voter_distr;
                        u.total_reward += voter_distr;
                    }); 

                    asset poolasset = (*pet).get_balance(name(VEXIOTOKEN), _self, VEX_SYMBOL.code());
                    if(voter_distr > poolasset)
                    {
                        voter_distr = poolasset;
                    }
                    if(voter_distr.amount > 0 && itrmd->voter != _self)
                    {
                        action(
                            permission_level{ _self, name("active")},
                            name(VEXIOTOKEN),
                            name("transfer"),
                            std::make_tuple(_self, itrmd->voter, voter_distr, std::string("bp vote reward"))
                        ).send();
                    }
                }
                else
                {
                    producer.calc_total_votes -= itrmd->voteweight; 
                    _producer.set(producer, _self);

                    _voters.erase(itrmd);
                }
            }
        }
        
        producer.checkfromid = id + 1;
        if((producer.checkfromid > producer.maxcheckid))
        {
            producer.checkfromid = 0;
            producer.checkStatu = 0;

            producer.lastdistramount = 0;
        }
        _producer.set(producer, _self);
    
    }
    

    void vexvotesplit::updatebp(const uint64_t &votesCounter)
    {
        auto producer = _producer.get_or_default();
        eosio_assert(producer.stopStatu == 0, "In stopStatu state!");
        require_auth(_self);
        
        struct producer_info {
            name                  owner;
            double                total_votes = 0;
            eosio::public_key     producer_key; /// a packed public key object
            bool                  is_active = true;
            std::string           url;
            uint32_t              unpaid_blocks = 0;
            time_point            last_claim_time;
            uint16_t              location = 0;

            uint64_t primary_key()const { return owner.value;                             }
            double   by_votes()const    { return is_active ? -total_votes : total_votes;  }
            bool     active()const      { return is_active;                               }
            void     deactivate()       { producer_key = public_key(); is_active = false; }
        };
        typedef eosio::multi_index< "producers"_n, producer_info,
                               indexed_by<"prototalvote"_n, const_mem_fun<producer_info, double, &producer_info::by_votes>  >
                             > producers_table;
        producers_table producers(name("vexcore"), name("vexcore").value);

        auto itrproducer = producers.find(producer.producer.value);
        eosio_assert(itrproducer != producers.end(), "producer not exist in producers table!");

        producer.maxcheckid = votesCounter - 1;
        producer.total_votes = itrproducer->total_votes;
        _producer.set(producer, _self);

    }

    void vexvotesplit::setstop(const uint8_t &state)
    {
        require_auth(_self);
        eosio_assert(state == 0 || state == 1, "set a wrong state!");
        auto producer = _producer.get_or_default();
        producer.stopStatu = state;
        _producer.set(producer,_self);
    }

    void vexvotesplit::onTransfer(eosio::name from,
                              eosio::name to,
                              eosio::extended_asset quantity,
                              std::string memo)
    {
        require_auth(from);

        if(isvextoken(quantity))
        {
            auto producer = _producer.get_or_default();
            eosio_assert(producer.checkfromid == 0, "last check not finish!");
            eosio_assert(now() - producer.lastdistrtime > SECONDS_23_HOUR, "Last distr not far from!");

            producer.lastdistramount = quantity.quantity.amount;
            producer.lastdistrtime = now();

            if(producer.scanStatu == 0 && producer.checkfromid == 0)
            {   
                producer.checkStatu = 1;
                producer.maxcheckid = producer.maxvoteid;
            }
            _producer.set(producer,_self);            
        }        
    }

    void vexvotesplit::init(const uint64_t &distrRate, const uint64_t &lastVotetime, const uint64_t &votesCounter, const name &bpName)
    {
        require_auth(_self);
        auto producer = _producer.get_or_default();
        eosio_assert(producer.initStatu != 1, "have init already!");

        producer.initStatu = 1;
        producer.stopStatu = 0;
        producer.scanStatu = 1;
        producer.producer = bpName;
        producer.total_votes = 0;
        producer.checkfromid = 0;
        producer.maxcheckid = 0;
        producer.distr_rate = distrRate;
        producer.highvotetime = lastVotetime;
        producer.lastvotetime = lastVotetime;
        producer.maxvoteid = 0;
        _producer.set(producer, _self);

        updatebp(votesCounter);
    }

};

