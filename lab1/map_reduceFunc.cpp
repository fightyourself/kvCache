#include <vector>
#include <string>
#include <utility>
using namespace std;

vector<string> split_into_words(const string& content){
    vector<string> res;
    string tmp;
    for(auto c : content){
        if(!isalpha(c)){
            if(tmp.size()>0){
                res.push_back(move(tmp));
                tmp.clear();
            }
        }else{
            tmp.push_back(c);
        }
    }
    if(tmp.size()>0) res.push_back(move(tmp));
    return res;
}

vector<pair<string,string>> map_func(const string &filename,const string &content){
    vector<pair<string,string>> res;
    auto words = split_into_words(content);
    for(const auto &word : words){
        res.push_back(make_pair(word,"1"));
    }
    return res;
}

string reduce_func(const string &key,const vector<string> &values){
    return to_string(values.size());
}