//libscry by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/libscry/blob/main/LICENSE

#include "card.h"

using namespace std;

Card::Card(const char * rawjson) {
  data.Parse(rawjson);
  if (strcmp(data["object"].GetString(), "error") == 0) throw "Invalid Card";
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

string Card::set() {
  return data["set_name"].GetString();
}

string Card::price() {
  return data["prices"]["usd"].GetString();
}

vector<string> Card::legality() {
  vector<string> output;
  output.push_back(data["legalities"]["standard"].GetString());
  output.push_back(data["legalities"]["future"].GetString());
  output.push_back(data["legalities"]["historic"].GetString());
  output.push_back(data["legalities"]["gladiator"].GetString());
  output.push_back(data["legalities"]["pioneer"].GetString());
  output.push_back(data["legalities"]["explorer"].GetString());
  output.push_back(data["legalities"]["modern"].GetString());
  output.push_back(data["legalities"]["legacy"].GetString());
  output.push_back(data["legalities"]["pauper"].GetString());
  output.push_back(data["legalities"]["vintage"].GetString());
  output.push_back(data["legalities"]["penny"].GetString());
  output.push_back(data["legalities"]["commander"].GetString());
  output.push_back(data["legalities"]["brawl"].GetString());
  output.push_back(data["legalities"]["historicbrawl"].GetString());
  output.push_back(data["legalities"]["alchemy"].GetString());
  output.push_back(data["legalities"]["paupercommander"].GetString());
  output.push_back(data["legalities"]["duel"].GetString());
  output.push_back(data["legalities"]["oldschool"].GetString());
  output.push_back(data["legalities"]["premodern"].GetString());
  return output;
}
