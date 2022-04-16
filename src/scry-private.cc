//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "scry.h"

using namespace std;

vector<string> Scry::explode(const string& str, const char& ch) {
  string next;
  vector<string> result;
  for (string::const_iterator it = str.begin(); it != str.end(); it++) {
    if (*it == ch) {
      if (!next.empty()) {
        result.push_back(next);
        next.clear();
      }
    } else next += *it;
  }
  if (!next.empty()) result.push_back(next);
  return result;
}

string Scry::implode(const vector<string>& strs, const char& ch) {
  string result = "";
  for (auto it = strs.begin(); it != strs.end(); it++) {
    result += (*it) + ch;
  }
  return result;
}

string Scry::urlformat(string str) {
  regex space(" ");
  regex colon(":");
  regex lt("<");
  regex gt(">");
  str = regex_replace(str, space, "%20");
  str = regex_replace(str, colon, "%3A");
  str = regex_replace(str, lt, "%3C");
  str = regex_replace(str, gt, "%3E");
  return str;
}

string Scry::nameformat(string str) {
  regex apos("'");
  str = regex_replace(str, apos, "''");
  return str;
}

string Scry::cachecard(List * list) {
  string output = "";
  vector <Card *> cards = list->cards();
  vector<string> names;
  vector<string> data;
  for (int i = 0; i < cards.size(); i++) {
    string name = nameformat(cards[i]->name());
    output += name + "\n";
    names.push_back(name);
    data.push_back( nameformat(cards[i]->json()) );
  }
  da->db_exec("Cards", names, data);
  output.pop_back();
  return output;
}

List * Scry::allcards(List * list) {
  List * newlist;
  if (list->nextPage() != "") {
    Document doc; doc.Parse(list->json().c_str());
    unsigned int pages = static_cast<int>(
      ceil(
	doc["total_cards"].GetInt() / (list->cards().size())
      )
    );
#ifdef DEBUG
    cerr << "# of pages: " << to_string(pages) << endl;
#endif
    vector<string> urls;
    int i;
    for (i = 2; i <= pages; i++) urls.push_back(list->nextPage() + to_string(i));
    vector<string> one; one.push_back(list->json());
    vector<string> two = wa->api_call(urls);
    two.insert(two.begin(), one.begin(), one.end());
    newlist = new List(two);
    lists.push_back(newlist);
    while (newlist->nextPage() != "") {
      string extrapage = wa->api_call(newlist->nextPage() + to_string(i));
      two.push_back(extrapage);
      newlist = new List(two);
      lists.push_back(newlist);
      i++;
    }
  } else newlist = list;
  return newlist;
}
