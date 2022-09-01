package restutils.objectMapper;

import com.google.gson.annotations.Expose;
import com.google.gson.annotations.SerializedName;


public class LocalEndpoint {

    @SerializedName("serviceName")
    @Expose
    private String serviceName;

    public String getServiceName() {
        return serviceName;
    }

    public void setServiceName(String serviceName) {
        this.serviceName = serviceName;
    }

}
