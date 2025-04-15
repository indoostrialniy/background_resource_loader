#pragma once

#ifndef RESOURCE_H
#define RESOURCE_H


#include "resourceManager.h"

// custom resource class example
class TestResource : public resource_manager::Resource	
{
public:
	TestResource( const std::string& Name );	// must-have constructor
	
	~TestResource();							// must-have destructor

private:
	void longLoading() override; 				// must-have func
	
	void quickConfiguring() override;			// must-have func
};



resource_manager::Manager <TestResource> 		TestResources("TestResource");


#endif
