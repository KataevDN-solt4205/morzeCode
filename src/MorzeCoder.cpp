/* при кодировании игнорируются всн символы кроме 'a-z' и пробела */

#include <iostream>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <exception>
#include <string.h>
#include <map>
#include "ICoder.hpp"
#include "MorzeCoder.hpp"

MorzeCoder::MorzeCoder() 
:   dot{'.', "", 1, 0b1},
    dash{'-', "", 3, 0b111},
    letter_delim{0, "",  3, 0b000},
    word_delim{0, " ", 7, 0b0000000},
    letter_to_word_delim{0, " ", 4, 0b0000},
    nil{0, "", 0, 0},
    simbols{
        {.simbol = 'a', .morze = ".-"  },
        {.simbol = 'b', .morze = "-..."},
        {.simbol = 'c', .morze = "-.-."},
        {.simbol = 'd', .morze = "-.." },
        {.simbol = 'e', .morze = "."   },
        {.simbol = 'f', .morze = "..-."},
        {.simbol = 'g', .morze = "--." },
        {.simbol = 'h', .morze = "...."},
        {.simbol = 'i', .morze = ".."  },
        {.simbol = 'j', .morze = ".---"},
        {.simbol = 'k', .morze = "-.-" },
        {.simbol = 'l', .morze = ".-.."},
        {.simbol = 'm', .morze = "--"  },
        {.simbol = 'n', .morze = "-."  },
        {.simbol = 'o', .morze = "---" },
        {.simbol = 'p', .morze = ".--."},
        {.simbol = 'q', .morze = "--.-"},
        {.simbol = 'r', .morze = ".-." },
        {.simbol = 's', .morze = "..." },
        {.simbol = 't', .morze = "-"   },
        {.simbol = 'u', .morze = "..-" },
        {.simbol = 'v', .morze = "...-"},
        {.simbol = 'w', .morze = ".--" },
        {.simbol = 'x', .morze = "-..-"},
        {.simbol = 'y', .morze = "-.--"},
        {.simbol = 'z', .morze = "--.."}
    }
{
    /* fill simbols bit codes*/
    for (int i = 0; i < simbols_len; i++) 
    {
        if (0 != FillCodeFromMorzeStr(simbols[i])){   
            throw std::length_error("Error on fill morze binary codes");
        }
        codes[simbols[i].code] = &simbols[i];
    } 
}

MorzeCoder::~MorzeCoder()
{

}

int MorzeCoder::Encode(std::string &src, std::vector<uint8_t> &dst)
{
    uint8_t  dst_byte = 0;				
    uint32_t dst_bit_offset = 0;
    const simbol_code_t *cur_word_delim = &word_delim;
                    
    simbol_code_t *morze_simbol;

    dst.clear();

    for (auto c: src) {                
        morze_simbol = NULL;
        if ((c >= 'A') && (c <= 'Z')){
            morze_simbol = &simbols[c-'A'];
        }
        else if ((c >= 'a') && (c <= 'z')){
            morze_simbol = &simbols[c-'a'];
        }        
        /* between word 1 delim */
        else if ((c == ' ') && (cur_word_delim != &word_delim)){
            EncodeSimbol(*cur_word_delim, dst, dst_byte, dst_bit_offset);
            cur_word_delim = &word_delim;
            continue;
        }
        else continue;

        EncodeSimbol(*morze_simbol, dst, dst_byte, dst_bit_offset);
        EncodeSimbol(letter_delim, dst, dst_byte, dst_bit_offset);
        cur_word_delim = &letter_to_word_delim;
    }
    if (cur_word_delim != &word_delim){
        EncodeSimbol(word_delim, dst, dst_byte, dst_bit_offset);
    }
    
    if (dst_bit_offset > 0){
        dst.push_back(dst_byte);
    }

    return 0;
}

int MorzeCoder::Decode(std::vector<uint8_t> &src, std::string &dst)
{
    uint32_t dst_code   = 0;
    uint32_t dst_offset = 0;  
    uint32_t zero_count = 0; 
    try 
    {
        for (uint8_t c: src) 
        {
            for (int i = 0; i<bits_in_byte; i++ )
            {
                if ((c >> i) & 1)
                {           
                    if (zero_count == letter_delim.code_len){
                        DecodeSimbol(dst, dst_code, dst_offset);
                    }
                    else if (zero_count >= word_delim.code_len){                    
                        DecodeSimbol(dst, dst_code, dst_offset);
                        dst += ' ';
                    } 
                    // /* если слиплось пару посылок(остаточные нули в байте)*/
                    // /* код должен начинаться с 1 сбросим сдвиг */
                    // else if ((dst_code == 0) && (dst_offset > 0)){
                    //     dst_offset = 0;
                    // }

                    zero_count = 0;                
                    dst_code |=  1 << dst_offset;
                }
                else 
                {
                    zero_count++;
                    // if (zero_count >= word_delim.code_len)
                    // {
                    //     if (dst_code){                        
                    //         DecodeSimbol(dst, dst_code, dst_offset);
                    //     }                     
                    //     dst += ' ';
                    //     dst_offset = 0;
                    //     zero_count = 0;
                    //     continue;
                    // }
                }
                dst_offset++;
            }
        }
        if (dst.length() > 1){
            dst.erase(dst.end()-1);
        }
    }
    catch (const std::runtime_error& error)
    {
        return -1;
    }
    return 0;
}

void MorzeCoder::EncodeBufToStr(std::vector<uint8_t> &buf, std::string &str)
{
    str.clear();
    for (uint8_t d: buf){
        for (int i = 0; i < bits_in_byte; i++){
            str += ((d >> i) & 0b1) ? "1" : "0";
        }
    }
}

int MorzeCoder::FillCodeFromMorzeStr(simbol_code_t &simbol)
{
    uint32_t max_pos = sizeof(simbol.code)*bits_in_byte;
    simbol.code     = 0;
    simbol.code_len = 0;
    for (char c: simbol.morze) 
    {
        const simbol_code_t *morze_simbol = &dot;
        if (c != dot.simbol){
            if (c == dash.simbol){
                morze_simbol = &dash;
            }
            else{
                 return -EINVAL;
            }
        }

        if ((morze_simbol->code_len == 0) || (max_pos < (morze_simbol->code_len + simbol.code_len))){
            return -EINVAL;
        }
        
        simbol.code |= morze_simbol->code << simbol.code_len;
        simbol.code_len += morze_simbol->code_len + 1;
    }

    /* del last delim & save */					
    simbol.code_len--;
    // std::cout << simbol.simbol << " : " << simbol.morze << " " <<  simbol.code_len;

    return 0;
}

void MorzeCoder::EncodeSimbol(const simbol_code_t &simbol, std::vector<uint8_t> &dst, uint8_t &dst_byte, uint32_t &dst_bit_offset)
{
    uint32_t src_bit_offset = 0;
    uint32_t copy_bit_len   = 0;

    /* пока не исчерпаем отправляемый символ */
    while ((simbol.code_len - src_bit_offset) > 0){

        copy_bit_len = std::min(simbol.code_len - src_bit_offset, bits_in_byte - dst_bit_offset);
        dst_byte |= ((simbol.code >> src_bit_offset) << dst_bit_offset) & (((1 << copy_bit_len) - 1) << dst_bit_offset);
        
        src_bit_offset += copy_bit_len;
        dst_bit_offset += copy_bit_len;

        if (dst_bit_offset == bits_in_byte){
            dst.push_back(dst_byte);
            dst_byte = 0;
            dst_bit_offset = 0;
        }
    }
}

void MorzeCoder::DecodeSimbol(std::string &dst, uint32_t &dst_code, uint32_t &dst_offset)
{
    std::map<uint32_t, simbol_code_t*>::const_iterator pos = codes.find(dst_code);
    if (pos == codes.end()) {  
        throw std::runtime_error("Wrong code in morze code");
    } else {
        dst += pos->second->simbol;
    }
    dst_code   = 0;
    dst_offset = 0;
}

void MorzeCoder::ShowSimbol(const simbol_code_t &simbol)
{
    printf("letter: %c, morze: %s, code_len: %d, code: 0b", simbol.simbol, simbol.morze.c_str(), simbol.code_len);
    for (uint32_t i = 0; i < simbol.code_len; i++){
        std::cout << (int)((simbol.code >> i) & 0b1);
    }
    std::cout << std::endl;
}
