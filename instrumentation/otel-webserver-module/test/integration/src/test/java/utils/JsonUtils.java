package utils;

import com.google.gson.JsonArray;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.Iterator;

public class JsonUtils {

    public void validateComplexJson(JSONObject json, String key) {
        boolean exists = json.has(key);
        Iterator<?> keys;
        String nextKeys;

        if(!exists) {
            keys = json.keys();
            while(keys.hasNext()){
                nextKeys = (String)keys.next();
                try{
                    if(json.get(nextKeys) instanceof JSONObject) {
                        if(!exists) {
                            validateComplexJson(json.getJSONObject(nextKeys), key);
                        } else if (json.get(nextKeys) instanceof JsonArray) {
                            JSONArray jsonArray = json.getJSONArray(nextKeys);
                            for(int i=0; i<jsonArray.length(); i++) {
                                String jsonArrayString = jsonArray.get(i).toString();
                                JSONObject innerJson = new JSONObject(jsonArrayString);
                                if (!exists) {
                                    validateComplexJson(innerJson, key);
                                }
                            }
                        }
                    }
                } catch (Exception e) {

                }
            }
        } else {
            parseObject(json, key);
        }
    }

    public void parseObject(JSONObject json, String key) {
        System.out.println(json.get(key));
    }
}