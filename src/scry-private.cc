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
  str = regex_replace(str, space, "%20");
  str = regex_replace(str, colon, "%3A");
  return str;
}

string Scry::nameformat(string str) {
  regex apos("'");
  str = regex_replace(str, apos, "''");
  return str;
}

string Scry::cachecard(List * list, bool recursive) {
  string names = "";
  vector <Card *> cards = allcards(list, true);
  if (recursive) {
    for (int i = 0; i < cards.size(); i++) {
      string name = nameformat(cards[i]->name());
      names += name + "\n";
      string temp = nameformat(cards[i]->json());
      if (i < list->cards().size()) {
        if (da->db_check("Cards", name)) {
          da->db_write("Cards", name, temp);
        } else da->db_new("Cards", name, temp);
      }
    }
  } else {
    for (int i = 0; i < cards.size(); i++) {
      string name = nameformat(cards[i]->name());
      names += name + "\n";
      string temp = nameformat(cards[i]->json());
      if (da->db_check("Cards", name)) {
        da->db_write("Cards", name, temp);
      } else da->db_new("Cards", name, temp);
    }
  }
  names.pop_back();
  return names;
}
