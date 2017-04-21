#ifndef TEST_CASE_H
#define TEST_CASE_H

#define ENABLE_TEST_DROP           (1)
#define ENABLE_TEST_EXCEPTION      (0)
#define ENABLE_EXCEPTION_LOG       (1)

#if ENABLE_TEST_DROP
#define DROP_RATE                  (2)
#define TEST_DROP                  if(std::rand()%(DROP_RATE) == (DROP_RATE-1)){continue;}
#else
#define TEST_DROP                  while(0)
#endif

#if ENABLE_TEST_EXCEPTION
#define ENABLE_CRITICAL_EXCEPTIONS (0)
#define EXCEPTION_RATE             (20)
#define TEST_EXCEPTION(ex)         do{if(std::rand()%(EXCEPTION_RATE) == (EXCEPTION_RATE-1))throw ex;}while(0)
#else
#define TEST_EXCEPTION(ex)         while(0)
#endif

#if ENABLE_EXCEPTION_LOG
#define EXCEPTION_PRINT            std::cout<<__PRETTY_FUNCTION__<<":"<<__LINE__<<":"<<ex.what()<<std::endl
#else
#define EXCEPTION_PRINT            while(0)
#endif

#endif
