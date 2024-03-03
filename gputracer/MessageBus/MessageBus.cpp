/*
  ==============================================================================

    MessageBus.cpp
    Created: 16 May 2018 8:52:48pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#include "MessageBus.h"

MessageBus* MessageBus::instance = NULL;

template <typename T>
Topic<T>* MessageBus::getTopic(std::string name) {
    if (topics.find( name ) != topics.end()) {
        return topics[name];
    }
    else{
        Topic* t = new Topic();
        t->setName(name);
        topics[name] = t;
        return t;
    }
}

void MessageBus::addListener(std::string name, BusListener* listener) {
    this->listener[name].push_back(listener);
}

template <typename T>
void MessageBus::updateTopic(std::string name, T value) {
    
    Topic* t = getTopic(name);
    
    if (t->getValue() != value) {
        t->setValue(value);
        
        for (int i = 0; i < listener[name].size();i++) {
            listener[name].at(i)->topicChanged(t);
        }
    }   
}

std::vector<std::string> MessageBus::getTopics() {
    std::vector<std::string> topicList = std::vector<std::string>();
    
    for(std::map<std::string,Topic*>::iterator it = topics.begin(); it != topics.end();++it) {
        topicList.push_back(it->first);
    }
    
    return topicList;
}
