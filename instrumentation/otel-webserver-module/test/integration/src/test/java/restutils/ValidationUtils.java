package restutils;

import io.restassured.response.Response;
import org.json.JSONArray;
import org.json.JSONObject;
import org.testng.Assert;
import java.util.logging.Logger;
import static utils.Constants.*;


public class ValidationUtils extends BaseTest{

    private static final Logger LOGGER = Logger.getLogger(ValidationUtils.class.getName());
    public RestClient restClient = new RestClient();
    public Response response;

    public void valiateResponseWithKey(Response response, String key) {

    }

    public void verifyAllTraces() {
        response = restClient.getResponse(ZIPKIN_URL + TRACES);
        LOGGER.info(response.body().asString());
        JSONArray jsonArray = new JSONArray(response.getBody().asString());
        for(int i=0; i<jsonArray.length(); i++) {
            JSONArray traceArray = jsonArray.getJSONArray(i);
            for (int j=0; j<traceArray.length(); j++){
                JSONObject span = traceArray.getJSONObject(j);
                if(span.get("name").toString().contentEquals("/")){
                    Assert.assertTrue(span.get("kind").toString().contentEquals("SERVER"));
                } else {
                    Assert.assertTrue(span.get("kind").toString().contentEquals("CLIENT"));
                }
                Assert.assertTrue(span.getJSONObject("tags").get("service.namespace").toString().contentEquals("sample_namespace"));
                Assert.assertTrue(span.getJSONObject("localEndpoint").get("serviceName").toString().contentEquals("demoservice"));
                Assert.assertTrue(span.getJSONObject("tags").get("telemetry.sdk.language").toString().contentEquals("cpp"));
            }
        }

    }

    public void verifyAllSpans() {
        response =  restClient.getResponse(ZIPKIN_URL + SPANS);
        String spans = response.jsonPath().get("$").toString();
        System.out.println(spans);
        LOGGER.info(spans);
        JSONArray jsonArray = new JSONArray(response.getBody().asString());
        for(int i=0; i<jsonArray.length(); i++) {
            if(i==0){
                Assert.assertTrue(jsonArray.getString(i).contentEquals("/"));
            }
            else if(i==1){
                Assert.assertTrue(jsonArray.getString(i).contentEquals("mod_proxy.c_handler"));
            }
            else if(i==2){
                Assert.assertTrue(jsonArray.getString(i).contentEquals("mod_proxy_balancer.c_handler"));
            } else {
                Assert.assertTrue(false);
            }
        }
    }

    public void verifyAllServices() {
        response = restClient.getResponse(ZIPKIN_URL + SERVICES);
        LOGGER.info(response.body().jsonPath().get().toString());
        JSONArray jsonArray = new JSONArray(response.getBody().asString());
        Assert.assertTrue(jsonArray.getString(0).contentEquals("demoservice"));
    }

}
