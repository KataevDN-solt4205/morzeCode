#include <iostream>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <exception>
#include <string.h>
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
    }
}

MorzeCoder::~MorzeCoder()
{

}

int MorzeCoder::Encode(std::string &src, std::vector<uint8_t> &dst)
{
    uint8_t  dst_byte = 0;				
    uint32_t dst_bit_offset = 0;
                    
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
        else if (c == ' '){
            EncodeSimbol(letter_to_word_delim, dst, dst_byte, dst_bit_offset);
            continue;
        }
        else continue;

        EncodeSimbol(*morze_simbol, dst, dst_byte, dst_bit_offset);
        EncodeSimbol(letter_delim, dst, dst_byte, dst_bit_offset);
    }
    EncodeSimbol(letter_to_word_delim, dst, dst_byte, dst_bit_offset);
    
    if (dst_bit_offset > 0){
        dst.push_back(dst_byte);
    }

    for (uint8_t d: dst){
        for (int i = 0; i < 8; i++){
            std::cout << (int)((d >> i) & 0b1);
        }
    }
    std::cout << std::endl;

    return 0;
}

int MorzeCoder::Decode(std::vector<uint8_t> &src, std::string &dst)
{
    // for (auto c: src) {
    // 	c
    // 	std::cout << num << std::endl;
    // }
    return 0;
}

int MorzeCoder::FillCodeFromMorzeStr(simbol_code_t &simbol)
{
    uint32_t max_pos = sizeof(simbol.code)*8;
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
    const uint32_t bit_in_byte = 8;

    uint32_t src_bit_offset = 0;
    uint32_t copy_bit_len   = 0;

    /* пока не исчерпаем отправляемый символ */
    while ((simbol.code_len - src_bit_offset) > 0){

        copy_bit_len = std::min(simbol.code_len - src_bit_offset, bit_in_byte - dst_bit_offset);
        dst_byte |= ((simbol.code >> src_bit_offset) << dst_bit_offset) & (((1 << copy_bit_len) - 1) << dst_bit_offset);
        
        src_bit_offset += copy_bit_len;
        dst_bit_offset += copy_bit_len;

        if (dst_bit_offset == bit_in_byte){
            dst.push_back(dst_byte);
            dst_byte = 0;
            dst_bit_offset = 0;
        }
    }
}

void MorzeCoder::ShowSimbol(const simbol_code_t &simbol)
{
    printf("letter: %c, morze: %s, code_len: %d, code: 0b", simbol.simbol, simbol.morze.c_str(), simbol.code_len);
    for (uint32_t i = 0; i < simbol.code_len; i++){
        std::cout << (int)((simbol.code >> i) & 0b1);
    }
    std::cout << std::endl;
}
