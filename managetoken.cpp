#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio.token/eosio.token.hpp>
#include <eosio/singleton.hpp>
#include <string>
#include <vector>
#include <set>
using namespace eosio;
using namespace std;

class [[eosio::contract("managetoken")]] managetoken : public eosio::contract {
    private:
        //////////////////Contract Vars///////////
        symbol TRPM_Symbol=symbol("TRPM",4);
        symbol Main_Symbol=symbol("EOS",4);
        name main_eosio_contract_token="eosio.token"_n;
        name trpm_eosio_contract_token="eostrpmtoken"_n;
        asset TRPM_Unit=asset((uint64_t)(10000),TRPM_Symbol);
        /////////////////////////////////////////

        ///////////////Contract Tables////////////
       struct [[eosio::table]] configstruct {
            uint64_t TRPM_Price_amount=250;
	    };
        typedef eosio::singleton<"settings"_n, configstruct> settings_table;
        settings_table config;

         struct [[eosio::table]] sysadmins {
            set<name> admins;
	    };
        typedef eosio::singleton<"sysadmins"_n, sysadmins> system_admins;
        system_admins admins_table;

        struct  [[eosio::table]] user {
            name user;
            uint64_t primary_key() const { return user.value;}
        };
        using users_index = eosio::multi_index<"users"_n, user>;
        users_index users;

        struct  [[eosio::table]] porpusel {
            uint64_t key;
            name proposer;
            set <name> approveres;
            set <name> rejecters;
            string type;
            uint16_t approved;
            uint16_t rejected;
            uint64_t typeforsearch;
            string actionParams;
            uint64_t primary_key() const { return key;}
            uint64_t get_secondary_1() const {return typeforsearch;}
        };
        //using porpusel_index = eosio::multi_index<"porpusels"_n, porpusel,indexed_by<"bytypesearch"_n, const_mem_fun<porpusel, uint64_t, &porpusel::get_secondary_1>>>;
        using porpusel_index = eosio::multi_index<"porpusels"_n, porpusel>;
        porpusel_index propusels;
        ///////////////////////////////////////////////////////////////////////////

        ////////////////Contract msig Functions///////////////////////////////////
        void make_transfer(name from,name to,asset quantity, string memo) {
            if(memo=="sell")
            {
                token::transfer_action transfer(main_eosio_contract_token, {get_self(), "active"_n});
                transfer.send(from,to,quantity,"sold !");
                
            }
            else {
                token::transfer_action transfer(trpm_eosio_contract_token, {get_self(), "active"_n});
                transfer.send(from,to,quantity,"bought !");
            }
        }

        void issue_trpm(name issuer,asset quantity, string memo) {
          
                token::issue_action issue(trpm_eosio_contract_token, {get_self(), "active"_n});
                issue.send(issuer,quantity,"issued");
        }


        void updtrpmprice( uint64_t new_TRPM_Price ) {

            auto state =config.get();
            state.TRPM_Price_amount=new_TRPM_Price;
            config.set(state,get_self());
        }


        void addnewadmin(name admin ) {
            auto state =admins_table.get();
            state.admins.insert(admin);
            admins_table.set(state,get_self());
        }
        /////////////////////////////////////////////////////////

        /////////////////helper functions///////////////////////    
        void add_user(name user) {
            auto iterator =users.find(user.value);
            if( iterator == users.end() )
            {
                users.emplace(get_self(), [&]( auto& row ) {
                    row.user = user;
                });
            }
        }
       

        void buytrpm(name from,name to ,asset quantity,string ref_user){
            
            check(quantity.amount >= 250,"mustn't be  less than 0.025 EOS");
            check(quantity.symbol == Main_Symbol, "These are not the droids you are looking for.");
             auto state=config.get();
    
            asset TRPM_Price=asset(state.TRPM_Price_amount,Main_Symbol);
            if(ref_user!=""){
                name refuser=name(ref_user);
                print(refuser," ",ref_user);
               
                auto iterator =users.find(refuser.value);
                if( iterator == users.end() )
                {
                    print("no ref account found");
                    return ;
                }
                asset trpm_quantity=asset(quantity.amount*TRPM_Unit.amount/TRPM_Price.amount,TRPM_Symbol);
                asset trpm_quantity_ref_user=asset(trpm_quantity.amount*10/100,TRPM_Symbol);
                make_transfer(get_self(),from,trpm_quantity,"buy");
                make_transfer(get_self(),refuser,trpm_quantity_ref_user,"buy");
                add_user(from);
            }
            else {
                
                asset trpm_quantity=asset(quantity.amount*TRPM_Unit.amount/TRPM_Price.amount,TRPM_Symbol);
                make_transfer(get_self(),from,trpm_quantity,"buy");
                add_user(from);
            }
        }
        void selltrpm(name from,name to,asset quantity,string memo){
          
            check(quantity.amount >= 40000, "mustn't be  less than 4000 TRPM");
            check(quantity.symbol == TRPM_Symbol, "These are not the droids you are looking for.");
             auto state=config.get();
            asset TRPM_Price=asset(state.TRPM_Price_amount,Main_Symbol);
            asset main_quantity=asset(quantity.amount*TRPM_Price.amount/TRPM_Unit.amount,Main_Symbol);
            make_transfer(get_self(),from,main_quantity,"sell");
        }

        void init() {
            configstruct default_config;
            // This way, multi-initialisation simply does nothing as the singleton already exists.
            config.get_or_create(_self, default_config);
             sysadmins default_admins;
            // This way, multi-initialisation simply does nothing as the singleton already exists.
            admins_table.get_or_create(_self, default_admins);
            
	    }
       ////////////////////////////////////////////////////////////////////////////


        public:
        managetoken(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds)
        ,config(get_self(), get_self().value),admins_table(get_self(),get_self().value)
        ,propusels(get_self(),get_self().value),users(get_self(),get_self().value) {
            init();
        }


        [[eosio::on_notify("*::transfer")]]
        void onreceivedtokens(name from,name to, asset quantity,string memo)
        {
            
            if (from == get_self() || to != get_self() ||(get_first_receiver()!=main_eosio_contract_token && get_first_receiver()!=trpm_eosio_contract_token))
            {
                return;
            }

            vector <string> memoItems;
            for(int i=0;i<memo.size();i++)
            {
                int j=i;
                string itemValue="";
                for(;memo[j]!=','&&j<memo.size();j++)
                    itemValue+=memo[j];
                
                i=j;
                memoItems.push_back(itemValue);
            }
       
            if(memoItems[0]=="buy_TRPM")
                buytrpm(from,to,quantity,memoItems[1]);
            else if(memoItems[0] =="sell_TRPM")
                selltrpm(from,to,quantity,memoItems[1]);
            return;
            
        }
        ///////////////testing functon/////////////////////
         [[eosio::action]]
            void printstuff( ) {
                auto state=config.get();
                asset x=asset(state.TRPM_Price_amount,Main_Symbol);
                auto state2=admins_table.get();
            print(x);
            print(TRPM_Unit);
        }
     
       
          [[eosio::action]]
            void propusle(name admin,string type,string actionParams) {
                require_auth(admin);
                auto state=admins_table.get();
                check(state.admins.find(admin)!=state.admins.end(),"you are not admin");
                set<name> temp;
                set<name> empty_vec;
                temp.insert(admin);
                propusels.emplace(admin, [&]( auto& row ) {
                    row.key = propusels.available_primary_key();
                    row.proposer=admin;
                    row.type=type;
                    row.approveres=temp;
                    row.rejecters=empty_vec;
                    row.approved=1;
                    row.rejected=0;
                    row.typeforsearch=0;
                    row.actionParams=actionParams;
                });
        }
        [[eosio::action]]
            void approve(name admin,uint64_t propusle_key) {
                require_auth(admin);
                auto it=propusels.find(propusle_key);
                check(it!=propusels.end(),"there is no propusle with the key that it given");
                auto state=admins_table.get();
                check(state.admins.find(admin)!=state.admins.end(),"you are not admin");
                check(it->approveres.find(admin)==it->approveres.end(),"you already a approved to this propusle");
                check(it->rejecters.find(admin)==it->rejecters.end(),"you already a reject to this propusle");
                uint16_t typeforsearch=0;
                if(it->approved+1 > state.admins.size()/2)
                    typeforsearch=1;
                propusels.modify(it,admin, [&]( auto& row ) {
                    row.approveres.insert(admin);
                    row.approved++;
                    row.typeforsearch=typeforsearch;
                });
        }
        [[eosio::action]]
            void reject(name admin,uint64_t propusle_key) {
                require_auth(admin);
                auto it=propusels.find(propusle_key);
                check(it!=propusels.end(),"there is no propusle with the key that it given");
                auto state=admins_table.get();
                check(state.admins.find(admin)!=state.admins.end(),"you are not admin");
                check(it->rejecters.find(admin)==it->rejecters.end(),"you already a reject to this propusle");
                check(it->approveres.find(admin)==it->approveres.end(),"you already a approved to this propusle");
                uint16_t typeforsearch=0;
                if(it->rejected+1 > state.admins.size()/2)
                    typeforsearch=2;
                propusels.modify(it,admin, [&]( auto& row ) {
                    row.rejecters.insert(admin);
                    row.rejected++;
                    row.typeforsearch=typeforsearch;
                });
        }
         [[eosio::action]]
            void exec(name admin,uint64_t propusle_key) {
                require_auth(admin);
                auto it=propusels.find(propusle_key);
                check(it!=propusels.end(),"there is no propusle with the key that it given");
                auto state=admins_table.get();
                check(state.admins.find(admin)!=state.admins.end(),"you are not admin");
                check(it->proposer==admin,"you aren't the one who propse this action");
                check(it->typeforsearch!=0,"this action not approved yet");
                check(it->typeforsearch!=2,"this action rejected");
                check(it->typeforsearch!=3,"this action have been exectued");
                string actionType=it->type;
                if(actionType=="make_transfer")
                {
                    string param=it->actionParams;
                    vector<string>params;
                    string temp="";
                    for(int i=0;i<param.size();i++)
                    {
                        if(param[i]==','){
                            params.push_back(temp);
                            temp="";
                        }
                        else 
                        {
                            temp+=param[i];
                        }
                    }
                    params.push_back(temp);
                    uint64_t v=0;
                    for(int i=0;i<params[2].size();i++)
                        v*=10,v+=params[2][i]-'0';
                    asset quantity=asset(v,TRPM_Symbol);
                    make_transfer(name(params[0]),name(params[1]),quantity,params[3]);
                }
                else if(actionType=="issue_trpm")
                {
                     string param=it->actionParams;
                    uint64_t v=0;
                    for(int i=0;i<param.size();i++)
                        v*=10,v+=param[i]-'0';
                    asset quantity=asset(v,TRPM_Symbol);
                    issue_trpm(get_self(),quantity,"issue some");
                }
                else if(actionType=="addnewadmin")
                {
                   
                    addnewadmin(name(it->actionParams));
                }
                else if(actionType=="updtrpmprice")
                {
                     string param=it->actionParams;
                    uint64_t v=0;
                    for(int i=0;i<param.size();i++)
                        v*=10,v+=param[i]-'0';
                    updtrpmprice(v);
                }
                propusels.modify(it,admin, [&]( auto& row ) {
                    row.typeforsearch=3;
                });
        }

        ///////////////testing functon/////////////////////
        [[eosio::action]]
        void deleteall()
        {
            auto it1=propusels.begin();
            for(;it1!=propusels.end();)
            {
                propusels.erase(it1);
                it1=propusels.begin();
            }

            auto it2=users.begin();
            for(;it2!=users.end();)
            {
                users.erase(it2);
                it2=users.begin();
            }
            configstruct default_config;
            config.set(default_config,get_self());
             sysadmins default_admins;
            admins_table.set(default_admins,get_self());

        }

}; 