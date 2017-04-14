#ifndef TEST_CASE_H
#define TEST_CASE_H

#define TEST                (0)

#if TEST
// (1/rate) is the probability that the corresponding test case happen.
#define DROP_TEST(rate)            do{if(std::rand()%(rate) == (rate-1))return;}while(0)
#define EXCEPTION_TEST(ex, rate)   do{if(std::rand()%(rate) == (rate-1))throw ex;}while(0)
#define EXCEPTION_PRINT            std::cout<<__PRETTY_FUNCTION__<<":"<<__LINE__<<":"<<ex.what()<<std::endl
#else
#define DROP_TEST(rate)            while(0)
#define EXCEPTION_TEST(ex, rate)   while(0)
#define EXCEPTION_PRINT            while(0)
#endif

#endif
