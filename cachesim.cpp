#include"cachesim.hpp"
#include<math.h>
#include <stdint.h>
#include<iostream>
#include <sstream>

bool *valid_victim;
bool **pf;
bool *pf_vc;
bool *dirty_victm;
uint64_t *victim_store;
uint64_t **tagstore;
bool **valid_bit;
bool **dirty_bit;
uint64_t **victim;
uint64_t row;
uint64_t column;
int lines;
int ways;
int nott = 0;
uint64_t c ;
uint64_t b ;
uint64_t s ;
uint64_t v ;
uint64_t k ;
uint64_t tag;
int hit = 0;
uint64_t done;
uint64_t address;
uint64_t victim_hit;
int wb = 0;
uint64_t last_miss_block_address = 0;
int64_t pending_stride = 0;


//int lru_cache(uint64_t address);
uint64_t calc_index_address(uint64_t address);
uint64_t calc_tag_address (uint64_t address);
uint64_t reform_index_main_address (uint64_t new_address);
uint64_t reform_tag_main_address (uint64_t new_address);
uint64_t victim_form_address (uint64_t address);
uint64_t block_addr(uint64_t address);




void setup_cache(uint64_t C, uint64_t B, uint64_t S, uint64_t V, uint64_t K)
{
    
    c=C;
    b=B;
    s=S;
    v=V;
    k=K;
    lines = int(pow(2, (c-b-s)));
    ways = int(pow(2, s));
    
    
    tagstore = new uint64_t*[lines];
    for (int i = 0; i<lines; i++)
        tagstore[i] = new uint64_t[ways];
    
    for (int i = 0; i<lines; i++)
        for (int j=0; j<ways; j++)
            tagstore[i][j] = -1;
    
    if(v!=0)
    {
        victim_store = new uint64_t[v];  //initialise the vc;
        for (int i = 0; i<v; i++)
            victim_store[i] = -1;
    }
    
    
                            /////****INITIALIZING ARRAYS FOR STORAGE*******///////
    
    valid_bit = new bool*[lines];
    for (int i = 0; i<lines; i++)
        valid_bit[i] = new bool[ways];
    
    for (int i = 0; i<lines; i++)
        for (int j=0; j<ways; j++)
            valid_bit[i][j] = false;
    
    
    dirty_bit = new bool*[lines];
    for (int i = 0; i<lines; i++)
        dirty_bit[i] = new bool[ways];
    
    for (int i = 0; i<lines; i++)
        for (int j=0; j<ways; j++)
            dirty_bit[i][j] = false;
    
    pf = new bool *[lines];
    for (int i= 0; i<lines; i++)
        pf[i] = new bool[ways];
    
    for (int i = 0; i<lines; i++)
        for (int j=0; j<ways; j++)
            pf[i][j] = false;

    
    
    if(v!=0)
    {
        dirty_victm = new bool[v];  //initialise the dirty bit for vc
        for (int i = 0; i<v; i++)
            dirty_victm[i] = false;
        
        valid_victim = new bool[v];
        for (int i =0; i<v; i++)
            valid_victim[i] = false;
        
        pf_vc = new bool [v];
        for (int i=0; i< v; i++)
            pf_vc[i] = false;

    }
    

}


void cache_access(char rw, uint64_t address, cache_stats_t* p_stats)
{
    
    lines = int(pow(2, (c-b-s)));
    ways = int(pow(2, s));
    p_stats->accesses++;
    uint64_t index = calc_index_address(address);
    
    uint64_t tag = calc_tag_address(address);
    uint64_t new_address = address>>b;
    
    // if read instruction.
    if(rw == 'r')
    {
        p_stats->reads++;
        hit = 0;
        int empty_counter = 0;  // temporary counter created to check how many ways have not been filled
        for (int j =0; j<ways; j++)
        {
            if (valid_bit[index][j] == false) // every element has been initialised to -1; so if -1; counter++
                empty_counter++;
        }
        
        for (int j=0; j<ways; j++)
        {
                                //////****Is it there in the main cache******//////
            if(tagstore[index][j] == tag && valid_bit[index][j] ==true ) // if tag matches tagstores
            {
                
                uint64_t mru_temp = tagstore[index][j];
                bool mru_dirty = dirty_bit[index][j];
                if(pf[index][j] == true)
                    p_stats->useful_prefetches++;
                pf[index][j] = false;
                
                for (int i = j; i<(ways-1)-empty_counter ;i++) //makes sure that if there's empty space in the cache that order doesnt get affected. the last set stores mru, 0th stores lru.
                {
                    tagstore[index][i] = tagstore[index][i+1];
                }
                tagstore[index][(ways-1)-empty_counter] = mru_temp;
                
                for (int i = j; i<(ways-1)-empty_counter ;i++) //makes sure that if there's empty space in the cache that order doesnt get affected. the last set stores mru, 0th stores lru.
                {
                    dirty_bit[index][i] = dirty_bit[index][i+1];
                    pf[index][j] = pf[index][j+1];
                }
                dirty_bit[index][(ways-1)-empty_counter] = mru_dirty;
                pf[index][(ways-1)-empty_counter] = false;
                
                hit = 1;
                wb = 0;
                goto end_this;
            }
        }
        
        
        if(v!=0)
        {                                                   //// CHECKS THE VICITM CACHE ///////
            for (int j =0; j<v; j++)
            {
                if (victim_store[j] == new_address && valid_victim[j] == true)
                {
                    uint64_t temp1 = tagstore[index][0];
                    bool temp2_bit = dirty_victm[j];
                    bool temp1_bit = dirty_bit[index][0];
                    bool temp3_bit = pf[index][0];
                    temp1 = temp1<<(c-b-s);
                    temp1 = temp1 | index;
                    victim_store[j] = temp1;                        ///// SWAPPING ///////
                    dirty_victm[j] = temp1_bit;
                    if(pf_vc[j] == true)
                        p_stats->useful_prefetches++;
                    pf_vc[j] = false;
                    pf_vc[j] = temp3_bit;
                    for(int i = 0; i< ways-1; i++)
                    {
                        tagstore[index][i] = tagstore[index][i+1];
                        dirty_bit[index][i] = dirty_bit[index][i+1];
                        pf[index][i] = pf[index][i+1];
                    }
                    tagstore[index][ways-1] = tag;
                    dirty_bit[index][ways-1] = temp2_bit;
                    pf[index][ways-1] = false;
                    hit = 0;
                    goto end_this;
                    
                }
            }
        }
        
        
        
        for (int j=0; j<ways; j++)
        {
            if (valid_bit[index][j] == false)
            {
                tagstore[index][j]  = tag; // if empty, writes data in there
                hit = 0;
                wb = 0;
                valid_bit[index][j] = true;
                dirty_bit[index][j] = false;
                pf[index][j] = false;
                p_stats->vc_misses++;
                p_stats->read_misses_combined++;
                goto end_this;
            }
        }
        
        
                            ////****Not present in the L1 Cache****/////////
        
        
        if(empty_counter == 0)
        {
            if(v==0)
            {
                if(dirty_bit[index][0] == true)
                {
                    p_stats->write_backs++;
                    dirty_bit[index][0] = false;
                }
                
                for (int j = 0; j<ways-1; j++)
                {
                    tagstore[index][j] =tagstore[index][j+1];
                    dirty_bit[index][j] = dirty_bit[index][j+1];
                    pf[index][j] = pf[index][j+1];
                }
                tagstore[index][ways-1] = tag;
                dirty_bit[index][ways-1] = false;
                pf[index][ways-1] = false;
                
            }
           
            if(v!=0)
            {
                uint64_t temp3 = tagstore[index][0];
                bool temp3_bit = dirty_bit[index][0];
                bool temp4_bit = pf[index][0];
                temp3 = temp3<<(c-b-s);
                temp3 = temp3 | index;
                
                int vc_empty_ctr = 0;
                for (int j= 0; j<v; j++)
                    if(valid_victim[j] == false)
                        vc_empty_ctr++;
                
                for (int j=0; j<ways-1; j++)
                {
                    tagstore[index][j] = tagstore[index][j+1];
                    dirty_bit[index][j] = dirty_bit[index][j+1];
                    pf[index][j] = pf[index][j-1];
                }
                tagstore[index][ways-1] = tag;
                dirty_bit[index][ways-1] = false;
                pf[index][ways-1] = false;
                
                if (vc_empty_ctr == 0)                          // VC is full, kick someone out
                {
                    if(dirty_victm[0] == true)
                    {
                        p_stats->write_backs++;
                        dirty_victm[0] = false;
                    }
                    for (int j = 0; j<v-1; j++)
                    {
                        victim_store[j] = victim_store[j+1];
                        dirty_victm[j] = dirty_victm[j+1];
                        pf_vc[j] = pf_vc[j+1];
                    }
                    victim_store[v-1] = temp3;
                    dirty_victm[v-1] = temp3_bit;
                    pf_vc[v-1] = temp4_bit;
                    
                    
                    
                }
                
                else
                {
                    for(int j=0; j<v; j++)
                        if(valid_victim[j] == false )
                        {
                            victim_store[j] = temp3;
                            dirty_victm[j] = temp3_bit;
                            valid_victim[j] = true;
                            pf_vc[j] = false;
                            break;
                        }
                }
                
            }
            hit = 0;
            p_stats->vc_misses++;
            p_stats->read_misses_combined++;
        }
    }
    
    
    
    
    if (rw == 'w')                                  /// WRITE INSTRUCITON//////
    {
        p_stats->writes++;
        nott= 0;
        int empty_counter = 0;  // temporary counter created to check how many ways have not been filled
        for (int j =0; j<ways; j++)
        {
            if (valid_bit[index][j] == false) //
                empty_counter++;
        }
        
        for (int j=0; j<ways; j++)
        {
            //////****Is it there in the main cache******//////
            if(tagstore[index][j] == tag && valid_bit[index][j] ==1 ) // if tag matches tagstores
            {
                
                uint64_t mru_temp = tagstore[index][j];
                if(pf[index][j] == true)
                    p_stats->useful_prefetches++;
                pf[index][j] = false;
                for (int i = j; i<(ways-1)-empty_counter ;i++) //makes sure that if there's empty space in the cache that order doesnt get affected. the last set stores mru, 0th stores lru.
                {
                    tagstore[index][i] = tagstore[index][i+1];
                     dirty_bit[index][i] = dirty_bit[index][i+1];
                    valid_bit[index][i] = valid_bit[index][i+1];
                    pf[index][i] = pf[index][i+1];
                }
                tagstore[index][(ways-1)-empty_counter] = mru_temp;
                dirty_bit[index][(ways-1)-empty_counter] = true;
                valid_bit[index][(ways-1)-empty_counter] = true;
                pf[index][(ways-1)-empty_counter] = false;
                
                nott = 1;
                wb = 0;
                goto end_this;
            }
        }
        if (v!=0)
        {
            for (int j =0; j<v; j++)
            {
                if (victim_store[j] == new_address && valid_victim[j] == true)
                {
                    uint64_t temp1 = tagstore[index][0];
                    bool temp1_bit = dirty_bit[index][0];
                    bool temp3_bit = pf[index][0];
                    temp1 = temp1<<(c-b-s);
                    temp1 = temp1 | index;
                    if(pf_vc[j] == true)
                        p_stats->useful_prefetches++;
                    pf_vc[j] = false;
                    victim_store[j] = temp1;
                    dirty_victm[j] = temp1_bit;
                    pf_vc[j] = temp3_bit;
                    for(int i = 0; i< ways-1; i++)
                    {
                        tagstore[index][i] = tagstore[index][i+1];
                        dirty_bit[index][i] = dirty_bit[index][i+1];
                        pf[index][i] = pf[index][i+1];
                    }
                    tagstore[index][ways-1] = tag;
                    dirty_bit[index][ways-1] = true;
                    pf[index][ways-1] = false;
                    
                    nott = 0;
                    goto end_this;
                    
                }
            }
        }
        
        
        
        /////****Is the main cache empty*//////
        
        
        for (int j=0; j<ways; j++)
        {
            if (valid_bit[index][j] == false)
            {
                tagstore[index][j]  = tag; // if empty writes data in there
                nott = 0;
                wb = 0;
                valid_bit[index][j] = true;
                dirty_bit[index][j] = true;
                pf[index][j] = false;
                p_stats->vc_misses++;
                p_stats->write_misses_combined++;
                goto end_this;
            }
        }
        
        
        ////****Not present in the L1 Cache****/////////
        
        
        
        if(empty_counter == 0) // not present in the L1 cache
        {
            if(v==0)
            {
                if(dirty_bit[index][0] == true)
                {
                    p_stats->write_backs++;
                    dirty_bit[index][0] = false;
                }
                
                for (int j = 0; j<ways-1; j++)
                {
                    tagstore[index][j] =tagstore[index][j+1];
                    dirty_bit[index][j] = dirty_bit[index][j+1];
                    valid_bit[index][j] = valid_bit[index][j+1];
                    pf[index][j] = pf[index][j+1];
                }
                tagstore[index][ways-1] = tag;
                dirty_bit[index][ways-1] = true;
                valid_bit[index][ways-1] = true;
                pf[index][ways-1] = false;
                
            }
            
            if (v!= 0)
            {
                uint64_t temp3 = tagstore[index][0];
                bool temp3_bit = dirty_bit[index][0];
                bool temp4_bit = pf[index][0];
                temp3 = temp3<<(c-b-s);
                temp3 = temp3 | index;
                
                int vc_empty_ctr = 0;
                for (int j= 0; j<v; j++)
                    if(valid_victim[j] == false)
                        vc_empty_ctr++;
                
                for (int j=0; j<ways-1; j++)
                {
                    tagstore[index][j] = tagstore[index][j+1];
                    dirty_bit[index][j] = dirty_bit[index][j+1];
                    pf[index][j]  = pf[index][j+1];
                }
                tagstore[index][ways-1] = tag;
                dirty_bit[index][ways-1] = true;
                pf[index][ways-1] = false;
                
                
                if (vc_empty_ctr == 0)                          // VC is full, kick someone out
                {
                    if(dirty_victm[0] == true)
                    {
                        p_stats->write_backs++;
                        dirty_victm[0] = false;
                    }
                    for (int j = 0; j<v-1; j++)
                    {
                        victim_store[j] = victim_store[j+1];
                        dirty_victm[j] = dirty_victm[j+1];
                        pf_vc[j] = pf_vc[j+1];
                    }
                    victim_store[v-1] = temp3;
                    dirty_victm[v-1] = temp3_bit;
                    pf_vc[v-1] = temp4_bit;
                    
                }
                
                else
                {
                    for(int j=0; j<v; j++)
                        if(victim_store[j] == -1 )
                        {
                            victim_store[j] = temp3;
                            dirty_victm[j] = temp3_bit;
                            pf_vc[j] = false;
                            break;
                            
                        }
                }
                
            }
            nott = 0;
            p_stats->vc_misses++;
            p_stats->write_misses_combined++;
            

        }

    }
    
    
    end_this:
    

    if(hit ==0 && rw == 'r')
        p_stats->read_misses++;
    
    if (nott == 0 && rw == 'w')
        p_stats->write_misses++;
                                                //////******PREFETCHER BEGINS NOW********///////
    if((hit==0 && rw == 'r') || (nott==0 && rw == 'w' ))
    {
        uint64_t X = address;
        X = X>>b;
        X = X<<b;
        int64_t d = X- last_miss_block_address;
        last_miss_block_address = X;
        if(d == pending_stride)
        {
            for (int i=1; i<k+1; i++)
            {
                int64_t prefetch_address = X + i*pending_stride;
                uint64_t new_index = prefetch_address;
                new_index = new_index<<(64-c+s);
                new_index = new_index>>(64-c+s+b);
                uint64_t new_tag = prefetch_address>>(c-s);
                p_stats->prefetched_blocks++;
                uint64_t vc_address = prefetch_address>>b;
                int abc = 0;
                int xyz = 0;
            
                for (int j= 0; j<ways; j++)
                {
                    if (tagstore[new_index][j] == new_tag && valid_bit[new_index][j] == true)   /// ALREADY IN L1/////
                    {
                        abc =1;
                        break;
                    }
                }
                
                int empty_ctr = 0;
                for (int j=0; j<ways; j++)
                    if(valid_bit[new_index][j] == false)
                        empty_ctr++;
                
                if(empty_ctr !=0)
                {
                    for (int j = ways-empty_ctr ; j>0; j--)
                    {
                        valid_bit[new_index][j] = valid_bit[new_index][j-1];            ///// L1 IS EMPTY
                        tagstore[new_index][j] = tagstore[new_index][j-1];
                        dirty_bit[new_index][j] = dirty_bit[new_index][j-1];
                    }
                    tagstore[new_index][0] = new_tag;
                    valid_bit[new_index][0] = true;
                    dirty_bit[new_index][0] = false;
                    pf[new_index][0] = true;
                    xyz = 1;
                }
                
                if(empty_ctr==0)
                {
                    if(v!=0)
                    {
                        for(int j = 0; j<v; j++)
                        {
                            if(victim_store[j] == vc_address)
                            {
                                uint64_t temp12 = tagstore[new_index][0];           /// PRESENT IN VC, PERFORM APPROPRIATE SWAPPING
                                bool temp12_bit = dirty_bit[new_index][0];
                                bool temp13_bit = pf[new_index][0];
                                temp12 = temp12<<(c-b-s);
                                temp12 = temp12 | new_index;
                                bool temp33_bit = dirty_victm[j];
                                victim_store[j] = temp12;
                                dirty_victm[j] = temp12_bit;
                                pf_vc[j] = temp33_bit;
                                tagstore[new_index][0] = new_tag;
                                dirty_bit[new_index][0] = temp13_bit;
                                pf[new_index][0] = true;
                                abc = 1;
                                break;
                            }
                            
                        }

                    }
                
                if(abc!=1)
                {
                    if(empty_ctr ==0 )
                    {
                        {
                            if(v==0)
                            {
                                if(dirty_bit[new_index][0] == true)
                                {
                                    p_stats->write_backs++;
                                    dirty_bit[new_index][0] = false;
                                }
                                tagstore[new_index][0] = new_tag;
                                valid_bit[new_index][0] = true;
                                pf[new_index][0] = true;
                            }
                            
                            if(v!=0)
                            {
                                uint64_t temp20 = tagstore[new_index][0];
                                bool temp20_bit = dirty_bit[new_index][0];
                                bool temp40_bit = pf[new_index][0];
                                temp20 = temp20<<(c-b-s);
                                temp20 = temp20 | new_index;
                                
                                int vc_empty = 0;
                                for(int j =0; j<v; j++)                                 ///PRESENT IN VC, PERFORM APPROPRIATE SWAPPING
                                {
                                    if(victim_store[j] == -1)               //// PREFETCHED BLOCK AS THE LRU 
                                        vc_empty++;
                                    
                                }
                                tagstore[new_index][0] = new_tag;
                                dirty_bit[new_index][0] = false;
                                pf[new_index][0] = true;
                                if(vc_empty == 0)
                                {
                                    if(dirty_victm[0] == true)
                                    {
                                        p_stats->write_backs++;
                                        dirty_victm[0] = false;
                                    }
                                    for(int j = 0; j<v-1; j++)
                                    {
                                        victim_store[j] = victim_store[j+1];
                                        dirty_victm[j] = dirty_victm[j+1];
                                        pf_vc[j] = pf_vc[j+1];
                                    }
                                    victim_store[v-1] = temp20;
                                    dirty_victm[v-1] = temp20_bit;
                                    pf_vc[v-1] = temp40_bit;
                                    
                                }
                                
                                else
                                {
                                    victim_store[v-vc_empty]= temp20;
                                    dirty_victm[v-vc_empty] = temp20_bit;
                                    pf_vc[v-vc_empty] = temp40_bit; 

                                }


                            }

                        }
                        
                        
                    
                    }
                
                
                    }
                
                }
                
            }
        }
    pending_stride=d;
}
        
    
    

}
uint64_t calc_index_address(uint64_t address)
{
    uint64_t x  = address<<(64 - c + s);
    x = x>>(64 - c + s + b);
    return x;
}

uint64_t calc_tag_address (uint64_t address)
{
    return address>>(c - s);
}


uint64_t reform_tag_main_address (uint64_t new_address)
{
    uint64_t tag = new_address>>(c-b-s);
    return tag;
    
}

uint64_t reform_index_main_address (uint64_t new_address)
{
    new_address = new_address<<(64-c+s);
    uint64_t index = new_address>>(64-c+s);
    return index;
}

uint64_t block_addr(uint64_t address)
{
    uint64_t some_address = address>>b;
    some_address = some_address<<b;
    return some_address;
}

void complete_cache(cache_stats_t *p_stats)
{
    p_stats->hit_time = 2 + 0.2*s;
    p_stats->miss_penalty = 200;
    p_stats->bytes_transferred = pow(2,b)*p_stats->vc_misses + pow(2,b)*p_stats->write_backs + pow(2,b)*p_stats->prefetched_blocks;
    p_stats->misses = p_stats->read_misses +p_stats->write_misses;
    p_stats->miss_rate = (double)(p_stats->misses)/(p_stats->accesses);
    float vc_miss_rate = (double)(p_stats->vc_misses)/(p_stats->accesses);
    p_stats->avg_access_time =  p_stats->hit_time + (p_stats->miss_penalty)*vc_miss_rate;
    for (int i=0; i<ways; i++)
    {
        delete[] tagstore[i];
    }
    delete[] tagstore;

}
