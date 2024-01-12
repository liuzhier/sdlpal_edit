#ifndef PALERROR_H
#define PALERROR_H
#pragma once

#include <exception>
using namespace std;

struct errOpenfile : public exception
{
	const char* what() const throw ()
	{
		return "打开文件错";
	}
};

struct errDir : public exception
{
	const char* what() //const throw ()
	{
		return "系统文件缺失";
	}
};


#endif
