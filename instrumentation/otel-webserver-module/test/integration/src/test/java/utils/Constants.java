package utils;

public class Constants {

    public static final String TRACES= "/traces?lookback=1800000&limit=9";
    public static final String SERVICES= "/services";
    public static final String SPANS= "/spans?serviceName=demoservice";

    private static final String HTTP_PROTOCOL = "http";
    private static final String BASE_URL="localhost:9411/api/v2";
    public static final String ZIPKIN_URL = HTTP_PROTOCOL + "://" + BASE_URL;
    public static final String WEBSERVER_URL="http://localhost:9004/";
}