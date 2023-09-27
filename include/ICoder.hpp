#ifndef __ICODER_HPP__
#define __ICODER_HPP__

class IEncoder
{
    public:
        virtual int Encode(std::string &src, std::vector<uint8_t> &dst) = 0;
};

class IDecoder
{
    public:
        virtual int Decode(std::vector<uint8_t> &src, std::string &dst) = 0;
};

#endif /* __ICODER_HPP__ */