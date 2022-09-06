package restutils;

import io.restassured.RestAssured;
import io.restassured.response.Response;
import static io.restassured.RestAssured.given;

public class RestClient {

    public static Response getResponse(String Uri){
        given().when().get(Uri).then().log()
                .all()
                .assertThat()
                .statusCode(200);

        return RestAssured.given().get(Uri);
    }
}