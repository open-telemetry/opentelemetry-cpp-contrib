package restutils;

import io.restassured.RestAssured;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeSuite;

import static utils.Constants.WEBSERVER_URL;
import static utils.Constants.ZIPKIN_URL;

public class BaseTest {

    private String ZIPKIN_SERVICE_NAME = "zipkin";
    private LoadGenUtils loadGenUtils = new LoadGenUtils();

    @BeforeSuite
    public void setup() {
        RestAssured.baseURI = ZIPKIN_URL;
    }

    @BeforeClass
    public void generateLoad(){
        loadGenUtils.generateLoad(WEBSERVER_URL, 90);
    }
}
