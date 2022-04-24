//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "scry.h"

using namespace std;

vector<char*> Scry::explode(const char* str, const char& ch) {
#ifdef DEBUG
  cerr << "Exploding: " << str << endl;
#endif
  cstring_t next; next.len = 0; next.max = 0; next.str = NULL;
  vector<char*> result;
  for (size_t i = 0; i < strlen(str); i++) {
    if (str[i] == ch) {
      if (next.str) {
	string_cat(&next, '\0');
#ifdef DEBUG
	cerr << "Line " << result.size() << ": " << next.str << endl;
#endif
        result.push_back(next.str);
        next.len = 0; next.max = 0; next.str = NULL;
      }
    } else string_cat(&next, str[i]);
  }
  if (next.str) result.push_back(next.str);
  return result;
}

string Scry::implode(const vector<string>& strs, const char& ch) {
  string result = "";
  for (auto it = strs.begin(); it != strs.end(); it++) {
    result += (*it) + ch;
  }
  return result;
}

void Scry::replace(char* str, char c, const char* s) {
#ifdef DEBUG
  cerr << "Replacing: " << str << endl;
#endif
  size_t length = strlen(str);
  for (size_t i = 0; i < length; i++) {
    if (str[i] == c) {
      size_t length_s = strlen(s);
      char temp[length_s] = "";
      char prev[length_s] = "";
      strcat(prev, s);
      length += length_s-1;
      if (!str) {
	fprintf(stderr, "not enough memory: realloc returned null");
	exit(1);
      }
      str[i] = prev[0];
      for (size_t j = i; j < length; j+=length_s-1) {
	for (size_t k = 1; k < length_s; k++) {
	  temp[k] = str[j+k];
	  str[j+k] = prev[k];
	  prev[k] = temp[k];
	}
      }
      i += length_s-1;
    }
  }
#ifdef DEBUG
  cerr << "Replaced with: " << str << endl;
#endif
}

void Scry::firstupper(char* str) {
#ifdef DEBUG
   cerr << "First upper input: " << str << endl;
#endif
   for(int i = 0; i < strlen(str); i++) {
      if (i == 0 || str[i-1] == ' ' || str[i-1] == '-') {
         if(str[i] >= 'a' && str[i] <= 'z') {
            str[i] = (char)(('A'-'a') + str[i] );
         }
      }
   }
#ifdef DEBUG
   cerr << "First upper output: " << str << endl;
#endif
}

char* Scry::urlformat(const string& str) {
  char* output = (char*)malloc(sizeof(char)*str.length()*3+1);
  if (!output) {
    fprintf(stderr, "not enough memory: malloc returned null");
    exit(1);
  }
  strcpy(output, str.c_str());
  replace(output, ' ', "%20");
  replace(output, ':', "%3A");
  replace(output, '<', "%3C");
  replace(output, '>', "%3E");
#ifdef DEBUG
  cerr << "URL formatted to: " << output << endl;
#endif
  return output;
}

char* Scry::nameformat(const string& str) {
  char* output = (char*)malloc(sizeof(char)*str.length()*2+1);
  if (!output) {
    fprintf(stderr, "not enough memory: malloc returned null");
    exit(1);
  }
  strcpy(output, str.c_str());
  replace(output, '\'', "''");
  firstupper(output);
#ifdef DEBUG
  cerr << "Name formatted to: " << output << endl;
#endif
  return output;
}

char* Scry::cachecard(List* list) {
  cstring_t output; output.len = 0; output.max = 0; output.str = NULL;
  vector<Card*> cards = list->cards();
  vector<char*> names;
  vector<char*> data;
  for (int i = 0; i < cards.size(); i++) {
    char* name = nameformat(cards[i]->name());
    string_cat(&output, name);
    string_cat(&output, "\n"); 
    names.push_back(name);
    string json = cards[i]->json();
    char* cstr = new char[json.length()+1];
    strcpy(cstr, json.c_str());
    data.push_back(cstr);
  }
  da->db_exec("Cards", names, data);
  output.len--;
  return output.str;
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

void Scry::string_cat(cstring_t* dest, const char* src) {
  size_t newlen = strlen(src);
#ifdef DEBUG
  cerr << "String cat: " << dest->len << ", " << dest->max << ", " << newlen << endl;
#endif
  if (dest->max < newlen+dest->len) {
    dest->max = max(dest->max * 2, newlen+dest->len+1);
#ifdef DEBUG
    cerr << "Realloc to size: " << dest->max << endl;
#endif
    dest->str = (char*)realloc(dest->str, dest->max);
    if (!dest->str) {
      fprintf(stderr, "not enough memory: realloc returned null");
      exit(1);
    }
  }
  strcpy(dest->str + dest->len, src);
  dest->len += newlen;
}

void Scry::string_cat(cstring_t* dest, char c) {
#ifdef DEBUG
  cerr << "String cat: " << dest->len << ", " << dest->max << ", 1" << endl;
#endif
  if (dest->max < 1+dest->len) {
    dest->max = max(dest->max * 2, dest->len+2);
#ifdef DEBUG
    cerr << "Realloc to size: " << dest->max << endl;
#endif
    dest->str = (char*)realloc(dest->str, dest->max);
    if (!dest->str) {
      fprintf(stderr, "not enough memory: realloc returned null");
      exit(1);
    }
  }
  dest->str[dest->len] = c;
  dest->len += 1;
}
