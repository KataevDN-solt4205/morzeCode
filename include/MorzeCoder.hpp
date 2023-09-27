#ifndef __MORZE_CODER_HPP__
#define __MORZE_CODER_HPP__

class MorzeCoder: public IEncoder, public IDecoder 
{
    public:
        MorzeCoder();
        virtual ~MorzeCoder();
        int Encode(std::string &src, std::vector<uint8_t> &dst);
        int Decode(std::vector<uint8_t> &src, std::string &dst);

    private:  
        typedef struct 
        {
            char          simbol;	 /* ASCII small simbol */
            std::string   morze;	 /* morze string  */
            uint32_t      code_len;  /* morze code bit length */
            uint32_t      code;	     /* morze code in bits */
        } simbol_code_t;
        
        const simbol_code_t dot;
        const simbol_code_t dash;
        const simbol_code_t letter_delim;
        const simbol_code_t word_delim;
        const simbol_code_t letter_to_word_delim;
        const simbol_code_t nil;

        static const int simbols_len = 'z'-'a' + 1;
        simbol_code_t simbols[simbols_len];
        std::map<uint32_t, simbol_code_t*, std::less<uint32_t>> codes;
        static const int bits_in_byte = 8;

        int  FillCodeFromMorzeStr(simbol_code_t &simbol);
        void EncodeSimbol(const simbol_code_t &simbol, std::vector<uint8_t> &dst, uint8_t &dst_byte, uint32_t &dst_bit_offset);
        void DecodeSimbol(std::string &dst, uint32_t &dst_code, uint32_t &dst_offset);
        void ShowSimbol(const simbol_code_t &simbol);
};

#endif /* __MORZE_CODER_HPP__ */