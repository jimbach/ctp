/*
 * Copyright 2009-2013 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __VOTCA_KMC_EVENT_H_
#define __VOTCA_KMC_EVENT_H_

namespace votca { namespace kmc {
  
class Event {
    
public:
    
   Event(){};
   virtual ~Event(){};     

   virtual std::string Event_type() = 0;
   //virtual void OnExecute() = 0;
    
private:
    
};

 //void Event::OnExecute(){
     //*state
     //carrier move()
    //std::cout << "Carrying out event : " << Event->Event_type("electron transfer") << endl;
}
} 

#endif