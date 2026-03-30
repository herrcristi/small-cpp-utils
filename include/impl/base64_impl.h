#pragma once

#include "impl_common.h"

#include <array>
#include <optional>
#include <stdlib.h>
#include <string>
#include <vector>

namespace small::base64impl {

    //
    // get base64 char corresponding in alphabet
    //
    inline char get_base64char_at(unsigned int index)
    {
        // if ( index < 26 )     return char( index + 'A' );
        // else if ( index < 52 )return char( index + ('a' - 26) );
        // else if ( index < 62 )return char( index + '0' - 52 );
        // else if ( index == 62 )return '+';
        // else if ( index == 63 )return '/';
        // return 0;

        // clang-format off
        static char __base64[] =
        {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
        };
        // clang-format on

        return index <= 63 ? __base64[index] : 0;
    }

    //
    // get index of base64 char or -1 if it is not a base64 char
    //
    inline int get_indexof_base64char(unsigned char ch)
    {
        // if ( ch >= 'A' && ch <= 'Z' )     return (ch - 'A');
        // else if ( ch >= 'a' && ch <= 'z' )return (ch - 'a' + 26);
        // else if ( ch >= '0' && ch <= '9' )return (ch - '0' + 52);
        // else if ( ch == '+' )             return 62;
        // else if ( ch == '/' )             return 63;
        // return -1;

        // clang-format off
        static int __index_of[256] = {
            -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/,	
            -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/, -1/*.*/,	
            -1/* */, -1/*!*/, -1/*"*/, -1/*#*/, -1/*$*/, -1/*%*/, -1/*&*/, -1/*'*/, -1/*(*/, -1/*)*/, -1/***/, 62/*+*/, -1/*,*/, -1/*-*/, -1/*.*/, 63/*/*/,	
            52/*0*/, 53/*1*/, 54/*2*/, 55/*3*/, 56/*4*/, 57/*5*/, 58/*6*/, 59/*7*/, 60/*8*/, 61/*9*/, -1/*:*/, -1/*;*/, -1/*<*/, -1/*=*/, -1/*>*/, -1/*?*/,	
            -1/*@*/,  0/*A*/,  1/*B*/,  2/*C*/,  3/*D*/,  4/*E*/,  5/*F*/,  6/*G*/,  7/*H*/,  8/*I*/,  9/*J*/, 10/*K*/, 11/*L*/, 12/*M*/, 13/*N*/, 14/*O*/,	
            15/*P*/, 16/*Q*/, 17/*R*/, 18/*S*/, 19/*T*/, 20/*U*/, 21/*V*/, 22/*W*/, 23/*X*/, 24/*Y*/, 25/*Z*/, -1/*[*/, -1/*\*/, -1/*]*/, -1/*^*/, -1/*_*/,	
            -1/*`*/, 26/*a*/, 27/*b*/, 28/*c*/, 29/*d*/, 30/*e*/, 31/*f*/, 32/*g*/, 33/*h*/, 34/*i*/, 35/*j*/, 36/*k*/, 37/*l*/, 38/*m*/, 39/*n*/, 40/*o*/,	
            41/*p*/, 42/*q*/, 43/*r*/, 44/*s*/, 45/*t*/, 46/*u*/, 47/*v*/, 48/*w*/, 49/*x*/, 50/*y*/, 51/*z*/, -1/*{*/, -1/*|*/, -1/*}*/, -1/*~*/, -1/**/,	
            -1/*€*/, -1/**/,  -1/*‚*/, -1/*ƒ*/, -1/*„*/, -1/*…*/, -1/*†*/, -1/*‡*/, -1/*ˆ*/, -1/*‰*/, -1/*Š*/, -1/*‹*/, -1/*Œ*/, -1/**/, -1/*Ž*/, -1/**/,	
            -1/**/,  -1/*‘*/, -1/*’*/, -1/*“*/, -1/*”*/, -1/*•*/, -1/*–*/, -1/*—*/, -1/*˜*/, -1/*™*/, -1/*š*/, -1/*›*/, -1/*œ*/, -1/**/, -1/*ž*/, -1/*Ÿ*/,	
            -1/* */, -1/*¡*/, -1/*¢*/, -1/*£*/, -1/*¤*/, -1/*¥*/, -1/*¦*/, -1/*§*/, -1/*¨*/, -1/*©*/, -1/*ª*/, -1/*«*/, -1/*¬*/, -1/*­*/, -1/*®*/, -1/*¯*/,	
            -1/*°*/, -1/*±*/, -1/*²*/, -1/*³*/, -1/*´*/, -1/*µ*/, -1/*¶*/, -1/*·*/, -1/*¸*/, -1/*¹*/, -1/*º*/, -1/*»*/, -1/*¼*/, -1/*½*/, -1/*¾*/, -1/*¿*/,	
            -1/*À*/, -1/*Á*/, -1/*Â*/, -1/*Ã*/, -1/*Ä*/, -1/*Å*/, -1/*Æ*/, -1/*Ç*/, -1/*È*/, -1/*É*/, -1/*Ê*/, -1/*Ë*/, -1/*Ì*/, -1/*Í*/, -1/*Î*/, -1/*Ï*/,	
            -1/*Ð*/, -1/*Ñ*/, -1/*Ò*/, -1/*Ó*/, -1/*Ô*/, -1/*Õ*/, -1/*Ö*/, -1/*×*/, -1/*Ø*/, -1/*Ù*/, -1/*Ú*/, -1/*Û*/, -1/*Ü*/, -1/*Ý*/, -1/*Þ*/, -1/*ß*/,	
            -1/*à*/, -1/*á*/, -1/*â*/, -1/*ã*/, -1/*ä*/, -1/*å*/, -1/*æ*/, -1/*ç*/, -1/*è*/, -1/*é*/, -1/*ê*/, -1/*ë*/, -1/*ì*/, -1/*í*/, -1/*î*/, -1/*ï*/,	
            -1/*ð*/, -1/*ñ*/, -1/*ò*/, -1/*ó*/, -1/*ô*/, -1/*õ*/, -1/*ö*/, -1/*÷*/, -1/*ø*/, -1/*ù*/, -1/*ú*/, -1/*û*/, -1/*ü*/, -1/*ý*/, -1/*þ*/, -1/*ÿ*/,	
        };
        // clang-format on

        return __index_of[ch];
    }

    //
    // get base64 buffer needed size (without null ending char)
    //
    inline std::size_t get_base64_size(const std::size_t length)
    {
        return ((length + 2) / 3) * 4;
    }

    //
    // to base64 (buffer must be proper allocated using get_base64_size + 1)
    //
    inline bool tobase64(char* base64, const char* src, const std::size_t src_length)
    {
        unsigned int encoded_word = 0;
        int          count_bits   = 0;
        int          multiple     = 0;
        unsigned int base64_index = 0;

        char ch = 0;
        for (std::size_t i = 0; i < src_length; ++i, ++src) {
            encoded_word  = encoded_word << 8 | static_cast<unsigned char>(*src);
            count_bits   += 8;

            for (; count_bits >= 6;) {
                ++multiple;
                if (multiple >= 4) {
                    multiple = 0;
                }

                count_bits   -= 6;
                base64_index  = 0x3F & (encoded_word >> count_bits);

                ch        = get_base64char_at(base64_index);
                *base64++ = ch;
            }
        }

        // still left something
        if (multiple > 0) {
            encoded_word  = encoded_word << 8 | ((unsigned char)'\0');
            count_bits   += 8;

            count_bits   -= 6;
            base64_index  = 0x3F & (encoded_word >> count_bits);

            ch        = get_base64char_at(base64_index);
            *base64++ = ch;

            //  add '='
            for (int k = multiple; k < 3; k++) {
                *base64++ = '=';
            }
        }

        return true;
    }

    //
    // get aprox size
    //
    inline std::size_t get_decodedbase64_size(const std::size_t base64_length)
    {
        return ((base64_length + 3) / 4) * 3;
    }

    //
    // decode from base 64 with inline RFC 4648 validation
    // returns: decoded length (0 if invalid), sets decoded_buffer[0] = '\0' on error
    //
    inline std::optional<std::size_t> frombase64(char* decoded_buffer, const char* base64, const std::size_t base64_length)
    {
        // Quick validation of length multiple of 4
        if (base64_length % 4 != 0) {
            decoded_buffer[0] = '\0';
            return std::nullopt; // invalid: base64 length must be multiple of 4
        }

        std::size_t decoded_length = 0;
        char*       decoded_ptr    = decoded_buffer;

        int  decoded_word  = 0;
        int  count_bits    = 0;
        bool found_padding = false;
        int  padding_count = 0;

        for (std::size_t i = 0; i < base64_length; ++i, ++base64) {
            const unsigned char ch = static_cast<unsigned char>(*base64);

            if (ch == '=') {
                // Once we see padding, all remaining chars must be padding
                found_padding = true;
                ++padding_count;
                continue;
            }

            // No non-padding characters after padding characters (RFC 4648 compliance)
            if (found_padding) {
                decoded_buffer[0] = '\0';
                return std::nullopt; // invalid: non-padding character after padding
            }

            // Check if it's a valid base64 character
            int add = get_indexof_base64char(ch);
            if (add < 0) {
                decoded_buffer[0] = '\0';
                return std::nullopt; // invalid: invalid character
            }

            decoded_word  = decoded_word << 6 | add;
            count_bits   += 6;

            if (count_bits >= 8) {
                count_bits               -= 8;
                unsigned char decoded_ch  = (unsigned char)(0xFF & (decoded_word >> count_bits));

                *decoded_ptr++ = static_cast<char>(decoded_ch);
                ++decoded_length;
            }
        }

        // Validate padding rules: 0, 1, or 2 '=' at the end, not 3 or 4
        if (padding_count > 2) {
            decoded_buffer[0] = '\0';
            return std::nullopt; // invalid: too much padding
        }

        // After counting padding_count and valid chars
        // Validate padding count matches data alignment
        const std::size_t valid_chars = base64_length - padding_count; // Non-padding chars
        // const std::size_t data_groups = valid_chars / 4; // Complete groups
        const std::size_t extra_chars = valid_chars % 4; // Remaining chars

        // Expected padding by extra chars:
        //  extra=0: 0 padding (complete)
        //  extra=1: invalid (need at least 2 chars to decode)
        //  extra=2: 2 padding
        //  extra=3: 1 padding
        if (extra_chars == 0 && padding_count != 0) {
            decoded_buffer[0] = '\0';
            return std::nullopt; // No extra, no padding
        }
        if (extra_chars == 1) {
            decoded_buffer[0] = '\0';
            return std::nullopt; // Invalid
        }
        if (extra_chars == 2 && padding_count != 2) {
            decoded_buffer[0] = '\0';
            return std::nullopt; // Extra 2, need 2 padding
        }
        if (extra_chars == 3 && padding_count != 1) {
            decoded_buffer[0] = '\0';
            return std::nullopt; // Extra 3, need 1 padding
        }

        return std::make_optional(decoded_length);
    }
} // namespace small::base64impl