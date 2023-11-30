package restutils;

import io.restassured.RestAssured;

public class LoadGenUtils {

    public void generateLoad(String Uri, int count) {
        while(count > 0){
            RestAssured.given().get(Uri);
            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            count --;
        }

    }
}
