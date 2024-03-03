/*7
  ==============================================================================

    MessageBus.h
    Created: 16 May 2018 8:52:48pm
    Author:  Matthias Pueski

  ==============================================================================
*/

#pragma once

#include "Topic.h"
#include "BusListener.h"
#include <map>
#include <vector>

class MessageBus {
public:

    /// <summary>
    /// Return the singleton instance of this MessageBus.
    /// </summary>
    /// <returns>Single messagebus</returns>
    static MessageBus* getInstance() {
        if (instance == NULL) {
            instance = new MessageBus();
        }
        return instance;
        
    }
    
    static void destroy() {
        delete instance;
    }
    
    /// <summary>
    /// Find the topic with a specified name. If the topic does not exist, create
    /// a new one with the name and return it.
    /// </summary>
    /// <param name="name">The topics name</param>
    /// <returns>The topic in any case</returns>

    template <typename T>
    Topic<T>* getTopic(std::string name);
    std::vector<std::string> getTopics();

    template <typename T>
    void updateTopic(std::string name, T value);
    void addListener(std::string name, BusListener* listener);
    
private:
    
    MessageBus() {}
    
    ~MessageBus() {
        for(std::map<std::string,Topic<int>*>::iterator it = intTopics.begin(); it != intTopics.end();++it) {
            delete (*it).second;
        }
        for (std::map<std::string, Topic<float>*>::iterator it = floatTopics.begin(); it != floatTopics.end(); ++it) {
            delete (*it).second;
        }
        for (std::map<std::string, Topic<bool>*>::iterator it = boolTopics.begin(); it != boolTopics.end(); ++it) {
            delete (*it).second;
        }
        for (std::map<std::string, Topic<std::string>*>::iterator it = stringTopics.begin(); it != stringTopics.end(); ++it) {
            delete (*it).second;
        }

        intTopics.clear();
        floatTopics.clear();
        boolTopics.clear();
        stringTopics.clear();
    }
    
    static MessageBus* instance;

    std::map<std::string,std::vector<BusListener*>> listener;
    std::map<std::string,Topic<int>*> intTopics;
    std::map<std::string, Topic<float>*> floatTopics;
    std::map<std::string, Topic<bool>*> boolTopics;
    std::map<std::string, Topic<std::string>*> stringTopics;
   
};
