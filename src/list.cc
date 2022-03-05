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
    cout << "Last ten chars: " << rawjsons[i].substr(rawjsons[i].length()-10, 10) << endl;
    cout << "Size: " << rawjsons[i].size() << endl << "Capacity: " << rawjsons[i].capacity() << endl;
#endif
    construct(rawjsons[i].c_str());
  }
  data.Parse(str.c_str());
}

void List::construct(const char * rawjson) {
  data.Parse(rawjson);
#ifdef DEBUG
  if (data.IsObject()) cout << "JSON is valid" << endl;
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
    cout << "Regex 1: " << sm1[0] << endl << "Regex 2: " << sm2[0] << endl << "Regex 3: " << sm3[0] << endl;
#endif
    nextpage = string(sm1[0]) + string(sm2[0]) + string(sm3[0]).substr(0, sm3[0].length()-3);
  } else nextpage = "";
}

List::List(vector<Card *> input) {
  content = input;
  nextpage = nullptr;
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
