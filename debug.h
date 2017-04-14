#ifndef TEST_CASE_H
#define TEST_CASE_H

#define ENABLE_TEST_DROP        (0)
#define ENABLE_TEST_EXCEPTION   (0)
#define ENABLE_EXCEPTION_LOG    (0)

#if ENABLE_TEST_DROP
#define TEST_DROP(rate)            do{if(std::rand()%(rate) == (rate-1))return;}while(0)
#else
#define TEST_DROP(rate)            while(0)
#endif

#if ENABLE_TEST_EXCEPTION
#define TEST_EXCEPTION(ex, rate)   do{if(std::rand()%(rate) == (rate-1))throw ex;}while(0)
#if ENABLE_EXCEPTION_LOG
#define EXCEPTION_PRINT            std::cout<<__PRETTY_FUNCTION__<<":"<<__LINE__<<":"<<ex.what()<<std::endl
#else
#define EXCEPTION_PRINT            while(0)
#endif
#else
#define TEST_EXCEPTION(ex, rate)   while(0)
#define EXCEPTION_PRINT            while(0)
#endif

#endif
