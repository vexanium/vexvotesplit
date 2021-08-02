#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/singleton.hpp>
#include "eosio.token.hpp"

#define SECONDS_23_HOUR  82800

#define VEXIOTOKEN "vex.token"
#define VEX_SYMBOL eosio::symbol("VEX", 4)

using namespace eosio;
using eosio::time_point;

namespace vexvote {

    class [[eosio::contract]] vexvotesplit :public contract {

    public:
        vexvotesplit(name receiver, name code, datastream<const char *> ds) :
                contract(receiver, code, ds),
                _producer(_self, _self.value),
                _voters(_self, _self.value) {
                    eosio::token eos_token = eosio::token(_self,code,ds);
                    pet = &eos_token;
        }

        [[eosio::action]]
        void test()
        {
            require_auth(_self);

            _producer.remove( );

            auto itr = _voters.begin();
            while(itr != _voters.end()){
                itr = _voters.erase(itr);
            }        
        }

        ACTION scanvote(const name &voter, const uint64_t &createat, const uint64_t &voteid, const uint64_t &totalvotetime);

        ACTION checkvoters(const uint64_t &id);//const uint64_t &idfrom, const uint64_t &idto

        ACTION init(const uint64_t &distrRate, const uint64_t &lastVotetime, const uint64_t &votesCounter, const name &bpName);

        ACTION setstop(const uint8_t &state);

        void updatebp(const uint64_t &votesCounter);

        // void updatevoter(const name &voter, const uint64_t &createat);

        bool isvextoken(const eosio::extended_asset &quantity)
        {
            if ((quantity.contract == eosio::name(VEXIOTOKEN)) && (quantity.quantity.symbol == VEX_SYMBOL))
            {
                return true;
            }
            return false;
        }

        void onTransfer(eosio::name from,
                    eosio::name to,
                    eosio::extended_asset quantity,
                    std::string memo); 

        void apply(name code, name action);

        TABLE st_voter
        {
            uint64_t id = 0;
            name voter;
            double voteweight = 0;
            int64_t votetime = 0;

            asset total_reward = asset(0, VEX_SYMBOL);
            asset last_reward = asset(0, VEX_SYMBOL);
            uint64_t primary_key() const { return id; }
            uint64_t by_voter() const { return voter.value; }
            uint64_t by_voteweight()const { return ((voteweight > 0) ? static_cast<uint64_t>(-voteweight) : static_cast<uint64_t>(-1)); }
            uint64_t by_votetime()const { return ((votetime > 0) ? static_cast<uint64_t>(-votetime) : static_cast<uint64_t>(-1)); }
        };
        typedef eosio::multi_index<"voters"_n, st_voter, 
                                eosio::indexed_by<"voter"_n, eosio::const_mem_fun<st_voter, uint64_t, &st_voter::by_voter>>,
                                eosio::indexed_by<"voteweight"_n, eosio::const_mem_fun<st_voter, uint64_t, &st_voter::by_voteweight>>,
                                eosio::indexed_by<"votetime"_n, eosio::const_mem_fun<st_voter, uint64_t, &st_voter::by_votetime>>
                                > tb_voters;
        TABLE producer
        {
            uint8_t initStatu = 0;
            uint8_t stopStatu = 0;
            uint8_t scanStatu = 1;
            uint8_t checkStatu = 0;

            name producer;
            double total_votes = 0;
            double calc_total_votes = 0;
            uint64_t distr_rate = 0; // 0 - 10000 for 0% - 100% 
            uint64_t lastdistrtime = 0;
            uint64_t lastdistramount = 0;

            uint64_t checkfromid = 0;
            uint64_t maxcheckid = 0;
            uint64_t lastchecktime = 0;
            uint64_t lastvotetime = 0;
            uint64_t highvotetime = 0;

            uint64_t maxvoteid = 0;
            
        };
        typedef eosio::singleton< "producer"_n, producer > tb_producer;

        eosio::token *pet;
        
        tb_voters _voters;
        tb_producer _producer;
    };

    struct st_transfer
    {
        eosio::name from;
        eosio::name to;
        eosio::asset quantity;
        std::string memo;
    };

    void vexvotesplit::apply(eosio::name code, eosio::name action)
    {      
        auto &thiscontract = *this;
    
        if ( action == eosio::name("transfer") && (code == eosio::name(VEXIOTOKEN))) 
        {
            auto transfer_data = eosio::unpack_action_data<st_transfer>();
            if(transfer_data.to == _self && transfer_data.memo == std::string("part vote pay for voters"))
            {
                onTransfer(transfer_data.from, transfer_data.to, eosio::extended_asset(transfer_data.quantity, code), transfer_data.memo);
                return;
            }
            //print("  apply code eosio::name", code); //code 为那个账户触发的通知
        }

        if (code != _self)
            return;
        if( code == _self ) {
            switch(action.value) {
                case eosio::name("test").value: 
                    execute_action(_self, eosio::name(code), &vexvotesplit::test); 
                    break;
                case eosio::name("setstop").value: 
                    execute_action(_self, eosio::name(code), &vexvotesplit::setstop); 
                    break;
                case eosio::name("init").value: 
                    execute_action(_self, eosio::name(code), &vexvotesplit::init); 
                    break;
                case eosio::name("scanvote").value: 
                    execute_action(_self, eosio::name(code), &vexvotesplit::scanvote); 
                    break;
                case eosio::name("checkvoters").value: 
                    execute_action(_self, eosio::name(code), &vexvotesplit::checkvoters); 
                    break;
            }
        }
    }


    extern "C"
    {
        [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
            eosio::datastream<const char*> ds( nullptr, 0 );
            vexvotesplit p(eosio::name(receiver), eosio::name(code), ds);
            p.apply(eosio::name(code), eosio::name(action));
            eosio_exit(0);
        }
    }

} /// namespace eosio
