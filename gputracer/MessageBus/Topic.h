/*
  ==============================================================================

    Topic.h
    Created: 16 May 2018 8:52:28pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once


#include <string>
template <typename T>
class Topic {
  
public:
    
    void setName(std::string name);
    std::string getName();  
   

    Topic::Topic() {

    }

    Topic::~Topic() {

    }

    std::string getName() {
        return name;
    }

    void setName(std::string name) {
        this->name = name;
    }

    T getValue() {
        return value;
    }

    void setValue(T value) {
        this->value = value;
    }


private:
    std::string name;
   
    T value;
    
};
