//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "card.h"

using namespace std;

Card::Card(const char * rawjson) {
  data.Parse(rawjson);
}

string Card::name() {
  return data["name"].GetString();
}

string Card::mana_cost() {
  if (!data.HasMember("mana_cost")) return "";
  return data["mana_cost"].GetString();
}

string Card::type_line() {
  return data["type_line"].GetString();
}

string Card::oracle_text() {
  if (!data.HasMember("oracle_text")) return "";
  return data["oracle_text"].GetString();
}

string Card::power() {
  if (!data.HasMember("power")) return "";
  return data["power"].GetString();
}

string Card::toughness() {
  if (!data.HasMember("toughness")) return "";
  return data["toughness"].GetString();
}

bool Card::dual_sided() {
  return data.HasMember("card_faces");
}

string Card::json() {
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  data.Accept(writer);
  return buffer.GetString();
}

string Card::loyalty() {
  if (!data.HasMember("loyalty")) return "";
  return data["loyalty"].GetString();
}
