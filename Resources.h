#pragma once
#ifndef RESOURCE_H
#define RESOURCE_H

// This file contains description of all resource classes you want to use in project
// All this classes have to been inherited from base class "Resource" from "resouceManager.h"

#include <resourceManager.h>

#include <string>


class Test : public Resource	
{
public:
	Test( const std::string& meshName );		// must-have member
	
	~Test();									// must-have member
	
	void longLoading() override; 				// must-have member
	
	void quickConfiguring() override;			// must-have member

};


// Be sure to call the method TestManager.StatusTracker.Execute(); in every loop cycle!

extern Manager <Test> 					TestManager;


#endif
