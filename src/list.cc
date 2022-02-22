//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "list.h"

using namespace std;
using namespace rapidjson;

List::List(const char * rawjson) {
  Document doc;
  doc.Parse(rawjson);
  for (int i = 0; i < doc["data"].Size(); i++) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    doc["data"][i].Accept(writer);
    content.push_back(new Card(buffer.GetString()));
  }
  if (doc["has_more"].GetBool()) {
    string url = doc["next_page"].GetString();
    regex q("q=.*&");
    regex page("page=.*&q");
    smatch sm1; regex_search(url, sm1, q);
    smatch sm2; regex_search(url, sm2, page);
    nextpage = string(sm1[0]).substr(2, sm1[0].length()-2) + string(sm2[0]).substr(0, sm2[0].length()-2);
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
