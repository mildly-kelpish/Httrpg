#include <locale>
#include <cstdio>
#include <iostream>
#include <istream>
#include <vector>
#include <string>
#include "libraries/toml.hpp"
#include "libraries/httplib.h"
#include "libraries/json.hpp"
#include "libraries/argy.hpp"

using namespace std;  // yet still: i use std::endl (i technically dont even need to!)
using json = nlohmann::json;


std::string formatunder(std::string text)
{
    std::replace(text.begin(), text.end(), '_', ' ');
    return text;
}


void printloop(nlohmann::json pmap, int pltypetwo) {
  for (int i = 0; i < pmap.at("map").size(); i++) {
    cout << pmap.at("map").at(i) << std::endl; // the map must always have a "map" key.
  }
  if (pltypetwo == 1) {
    cout << "1: inspect" << "\n" << "2: message" <<"\n"<<"3: modify map" << "\n" << "4: create new object" << "\n" << "5: roll dice" << "\n" << "6: exit" << std::endl;
  } else {
    cout << "1: inspect" << "\n" <<"2: message" << "\n"  << "3: roll"<< "\n" <<"4: exit" << std::endl;
  }

}



int main(int argc,char* argv[] ) {
    Argy::CliParser cli(argc, argv);
    cli.addInt({"-p", "--ptype"}, "player type to authenticate as", 2);
    cli.addString({"-k", "--key"}, "key to authenticate with", "NONE");
    cli.addString({"-s", "--url"}, "server to connect to", "http://127.0.0.1:8000/");
    cli.addBool({"-t", "--toml"}, "configure via toml file (config.toml within same directory as program) instead of by arguments");
    cli.addBool({"-v", "--verbose"}, "dont clear screen and also log a couple extra things occasionally");
    auto args = cli.parse(); 
    cout  << "HTTRPG official client v1.0" << std::endl;
    toml::value config;
    std::string svadress;
    int ptype;
    std::string pkey;
    std::string atoken;
    if (args.getBool("toml")) {
        config = toml::parse("config.toml");
        svadress = config.at("SERVER").as_string();
        ptype = config.at("PLAYERTYPE").as_integer();
        pkey = config.at("PLAYERKEY").as_string();

        cout << "loaded config!" << std::endl;
    }
    else {
        cout << "proceeding without loading config..." << std::endl;
        svadress = args.getString("url");
        ptype = args.getInt("ptype");
        pkey = args.getString("key");
    }


    httplib::Client clie(svadress);
    cout << "connecting to server..." << std::endl; // why printf when cout << "string" << std::endl;
    int playerholder; // only gets used on ptype 2
    nlohmann::json authentication = {{"key", pkey}, {"typ", ptype}};
    cout << authentication.dump() << std::endl;
    auto res = clie.Post("/authenticate", authentication.dump(), "application/json");
    if (res && res->status == 200) {
         cout << "loading complete!" << std::endl;
         atoken = res->get_header_value("set-cookie");
         cout << res->body << std::endl;
         playerholder = stoi(res->body);
         clie.set_default_headers({
            {"Cookie", atoken}
         });
    } else {
        cout << "authentication failed!" << "\n" << res->body << std::endl;
        return 1;
    }
    res = clie.Get("/map");
    nlohmann::json map;
    
    if (res && res->status == 200) {
      if (!args.getBool("verbose")) {
        cout << "\E[H\E[2J";
      } 
      
        map = nlohmann::json::parse(res->body);
        if (args.getBool("verbose")) {
          cout << map.at("map").size() <<"x" << map.at("map").at(0).size() << std::endl; // orginially for testing, might as well print with verbose enabled
        }
        printloop(map, ptype);
    } else {
        cout << "unable to retrieve or display map!" << "\n" << res->body  << std::endl;
        return 1;
    }
    std::string objid;
    std::string objn;
    std::string objstats;
    nlohmann::json objectmake;
    int choice;
    int exiting = 0;
    if (ptype == 1) { // ptype 1 is the DM type
      while (exiting == 0){
        cin >> choice;
        while (cin.fail()) {
          cout << "invalid input!!" << std::endl;
          cin.clear();
          cin.ignore(1000, '\n');
          cin >> choice;
        }
        switch (choice) { 
          case 1: // inspect
            if (!args.getBool("verbose")) {
                cout << "\E[H\E[2J";
            } // i could do this much better looking if i had a way to "press enter to continue..."   but i dont so 
            cout << "please input object id to inspect (or \"msgl\")" << std::endl;
            cin >> objid;
            if (objid == "msgl") {
              res = clie.Get("/msg");
              cout << res->body << std::endl;
              printloop(map, ptype);
            } else {
              res = clie.Get("/info/" + objid);
              printloop(map, ptype);
              cout << objid << ": " << res->body << std::endl;
            }
            cin.ignore(1000, '\n');
            break;
          case 2: // message
            if (!args.getBool("verbose")) {
              cout << "\E[H\E[2J";
            }
            cout << "please input message to send (or \"CLR\" to clear messagelog) (instead of spaces, use underscores, the server replaces it as nessecary)" << std::endl;
            cin >> objid;
            if (objid == "CLR") {
              res = clie.Get("/msg/clear");
              cout << res->body << std::endl;
            } else {
              cout << "please input name to send message under" << std::endl;
              cin >> objn;
              objectmake = {{"playerID", 0},  {"content", objid}, {"undername", objn}};
              cout << objectmake.dump() << std::endl;
              res = clie.Post("/msg", objectmake.dump(), "application/json");
              cout << res->status << std::endl;
              
            }
            printloop(map, ptype);
            cin.ignore(1000, '\n');
            break;
          case 4: // create object
            if (!args.getBool("verbose")) {
              cout << "\E[H\E[2J";
            }
            cout << "please input object id to create" << std::endl;
            cin >> objid;
            cout << "please input object name" << std::endl;
            cin >> objn;
            cout << "please input stats (separated by commas without spaces)" << std::endl;
            cin >> objstats;
            objectmake = {{"names", objn}, {"status", objstats}};
            res = clie.Post("/make/" + objid, objectmake.dump(), "application/json");
            cout << res->status << std::endl;
            printloop(map, ptype);
            cin.ignore(1000, '\n');
            break;
          case 6: // exit
            cout << "exiting..." << std::endl;
            exiting = 1;
            break;
          case 5: // roll
            if (!args.getBool("verbose")) {
              cout << "\E[H\E[2J";
            }
            cout << "enter dice to roll (must be in XdY format, d must be lowercase)" << std::endl;
            cin >> objid;
            res = clie.Get("/roll/" + objid);
            printloop(map, ptype);
            cout << "rolled: " <<res->body << std::endl;
            cin.ignore(1000, '\n');
            break;
          case 3:
            if (!args.getBool("verbose")) {
              cout << "\E[H\E[2J";
            }
            cout << "enter JSON string for map editing (instead of spaces, use underscores, the server replaces it as nessecary)" << std::endl; // this is the best way i could think of to do this :c
            cout << "must follow certain formats, here is an example" << "\n" << "{\"mappng\": [[1,1,1,1,1], [1,1,1,1,1]]}" << std::endl;
            cin >> objid;
            objn = formatunder(objid);
            res = clie.Post("/map", objn, "application/json");
            cin.ignore(1000, '\n');
            printloop(map, ptype); 





        }
        res = clie.Get("/map");
        map = nlohmann::json::parse(res->body);
      }

    }
    if (ptype == 2) { // ptype 2 is the PLAYER type
      while (exiting == 0) {
        cin >> choice;
        while (cin.fail()) {
          cout << "invalid input!!" << std::endl;
          cin.clear();
          cin.ignore(1000, '\n');
          cin >> choice;
        }
        switch (choice) {
          case 1:
            if (!args.getBool("verbose")) {
              cout << "\E[H\E[2J";
            }
            cout << "please input object id to inspect (or \"msgl\")" << std::endl;
            cin >> objid;
            if (objid == "msgl") {
              cout << "reading messagelog" << std::endl;
              res = clie.Get("/msg");
              cout << res->body << std::endl;
              printloop(map, ptype);
            } else {
              res = clie.Get("/info/" + objid);
              printloop(map, ptype);
              cout << res->body << std::endl;
            }
            cin.ignore(1000, '\n');
            break;
          case 2:
            if (!args.getBool("verbose")) {
              cout << "\E[H\E[2J";
            }
            cout << "please input message to send (instead of spaces, use underscores, the server replaces it as nessecary)" << std::endl; // c++ I DEPSISE THEE! WHY CANT cin.getline() JUST WORKK
            cin >> objid; // i really just be using objid for everything
            objectmake = {{"playerID", playerholder}, {"undername", "none"}, {"content", objid}};
            res = clie.Post("/msg", objectmake.dump(), "application/json");
            cout << res->status << std::endl;
            printloop(map, ptype);
            cin.ignore(1000, '\n');
            break;
          case 3:
            if (!args.getBool("verbose")) {
              cout << "\E[H\E[2J";
            }
            cout << "enter dice to roll (must be in XdY format, d must be lowercase)" << std::endl;
            cin >> objid;
            res = clie.Get("/roll/"+ objid);
            printloop(map, ptype);
            cout << "rolled: " << res->body << std::endl;
            cin.ignore(1000, '\n');
            break;


          case 4:
            cout << "exiting..." << std::endl;
            exiting = 1;
            break;
        }
        res = clie.Get("/map");
        map = nlohmann::json::parse(res->body);        

      }

    }







    return 0;
}
