//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "scry.h"

using namespace std;

vector<char*> Scry::explode(const char* str, const char& ch) {
#ifdef DEBUG
  fprintf(stderr, "Exploding: %s\n", str);
#endif
  cstring_t next; next.len = 0; next.max = 0; next.str = NULL;
  vector<char*> result;
  for (size_t i = 0; i < strlen(str); i++) {
    if (str[i] == ch) {
      if (next.str) {
	string_cat(&next, '\0');
#ifdef DEBUG
	fprintf(stderr, "Line %d: %s\n", result.size(), next.str);
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
  fprintf(stderr, "Replacing: %s\n", str);
#endif
  size_t length = strlen(str);
  for (size_t i = 0; i < length; i++) {
    if (str[i] == c) {
      size_t length_s = strlen(s);
      char temp[length_s] = "";
      char prev[length_s] = "";
      strcat(prev, s);
      length += length_s-1;
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
  fprintf(stderr, "Replaced with: %s\n", str);
#endif
}

void Scry::firstupper(char* str) {
#ifdef DEBUG
   fprintf(stderr, "First upper input: %s\n", str);
#endif
   for(int i = 0; i < strlen(str); i++) {
      if (i == 0 || str[i-1] == ' ' || str[i-1] == '-') {
         if(str[i] >= 'a' && str[i] <= 'z') {
            str[i] = (char)(('A'-'a') + str[i] );
         }
      }
   }
#ifdef DEBUG
   fprintf(stderr, "First upper output: %s\n", str);
#endif
}

char* Scry::urlformat(const string& str) {
  char* output = (char*)calloc(str.length()*3, sizeof(char)*str.length()*3+1);
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
  fprintf(stderr, "URL formatted to: %s\n", output);
#endif
  return output;
}

char* Scry::nameformat(const string& str) {
  char* output = (char*)calloc(str.length()*2, sizeof(char)*str.length()*2+1);
  if (!output) {
    fprintf(stderr, "not enough memory: malloc returned null");
    exit(1);
  }
  strcpy(output, str.c_str());
  replace(output, '\'', "''");
  firstupper(output);
#ifdef DEBUG
  fprintf(stderr, "Name formatted to: %s\n", output);
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

List* Scry::allcards(List* list) {
  List* newlist;
  if (list->nextPage() != "") {
    Document doc; doc.Parse(list->json().c_str());
    unsigned int pages = static_cast<int>(
      ceil(
	doc["total_cards"].GetInt() / (list->cards().size())
      )
    );
#ifdef DEBUG
    fprintf(stderr, "# of pages: %d\n", pages);
#endif
    char** urls = (char**)malloc(sizeof(char*)*(pages-1));
    int i;
    for (i = 1; i < pages; i++) {
      urls[i-1] = (char*)calloc(list->nextPage().size()+4, list->nextPage().size()+4);
      sprintf(urls[i-1], "%s%d", list->nextPage().c_str(), i+1);
    }
    vector<char*> results = wa->api_call(urls, pages-1);
    for (int i = 0; i < pages-1; i++) free(urls[i]);
    free(urls);
    char str[list->json().size()+1] = "";
    strcat(str, list->json().c_str());
    results.insert(results.begin(), str);
    newlist = new List(results);
    lists.push_back(newlist);
    while (newlist->nextPage() != "") {
      char url[newlist->nextPage().size()+4] = "";
      sprintf(url, "%s%d", newlist->nextPage().c_str(), i);
      char* extrapage = wa->api_call(url);
      results.push_back(extrapage);
      newlist = new List(results);
      lists.push_back(newlist);
      i++;
    }
  } else newlist = list;
  return newlist;
}

void Scry::string_cat(cstring_t* dest, const char* src) {
  size_t newlen = strlen(src);
#ifdef DEBUG
  fprintf(stderr, "String cat: %d, %d, %d\n", dest->len, dest->max, newlen);
#endif
  if (dest->max < newlen+dest->len) {
    dest->max = max(dest->max * 2, newlen+dest->len+1);
#ifdef DEBUG
    fprintf(stderr, "Realloc to size: %d\n", dest->max);
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
  fprintf(stderr, "String cat: %d, %d, 1\n", dest->len, dest->max);
#endif
  if (dest->max < 1+dest->len) {
    dest->max = max(dest->max * 2, dest->len+2);
#ifdef DEBUG
    fprintf(stderr, "Realloc to size: %d\n", dest->max);
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
