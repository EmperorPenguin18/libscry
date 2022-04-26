//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "list.h"

using namespace std;
using namespace rapidjson;

List::List(const char * rawjson) {
  construct(rawjson);
}

List::List(vector<string> rawjsons) {
  string str = "";
  for (int i = 0; i < rawjsons.size(); i++) {
    str += rawjsons[i] + '\n';
#ifdef DEBUG
    fprintf(stderr, "Last ten chars: %s\n", rawjsons[i].c_str()+rawjsons[i].size()-10);
    fprintf(stderr, "Size: %d\nCapacity: %d\n", rawjsons[i].size(), rawjsons[i].capacity());
#endif
    construct(rawjsons[i].c_str());
  }
  data.Parse(str.c_str());
}

void List::construct(const char * rawjson) {
  data.Parse(rawjson);
  if (strcmp(data["object"].GetString(), "error") == 0) throw "Invalid List";
#ifdef DEBUG
  if (data.IsObject()) fprintf(stderr, "JSON is valid\n");
#endif
  for (int i = 0; i < data["data"].Size(); i++) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    data["data"][i].Accept(writer);
    content.push_back(new Card(buffer.GetString()));
  }
  if (data["has_more"].GetBool()) {
    string url = data["next_page"].GetString();
    regex dmn(".*\\?");
    regex q("q=.*&");
    regex page("page=.*&q");
    smatch sm1; regex_search(url, sm1, dmn);
    smatch sm2; regex_search(url, sm2, q);
    smatch sm3; regex_search(url, sm3, page);
#ifdef DEBUG
    fprintf(stderr, "Regex 1: %s\nRegex 2: %s\nRegex 3: %s\n", sm1[0].str().c_str(), sm2[0].str().c_str(), sm3[0].str().c_str());
#endif
    nextpage = string(sm1[0]) + string(sm2[0]) + string(sm3[0]).substr(0, 5);
  } else nextpage = "";
}

List::~List() {
  while (!content.empty()) {
    delete content.back();
    content.pop_back();
  }
}

vector<Card *> List::cards() {
  return content;
}

string List::nextPage() {
  return nextpage;
}

string List::json() {
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  data.Accept(writer);
  return buffer.GetString();
}
