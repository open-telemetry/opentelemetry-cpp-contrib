package restutils.objectMapper;


import com.google.gson.annotations.Expose;
import com.google.gson.annotations.SerializedName;

public class Tags {

    @SerializedName("cloud.zone")
    @Expose
    private String cloudZone;
    @SerializedName("host.name")
    @Expose
    private String hostName;
    @SerializedName("interactionType")
    @Expose
    private String interactionType;
    @SerializedName("otel.library.name")
    @Expose
    private String otelLibraryName;
    @SerializedName("otel.library.version")
    @Expose
    private String otelLibraryVersion;
    @SerializedName("otel.status_code")
    @Expose
    private String otelStatusCode;
    @SerializedName("service.instance.id")
    @Expose
    private String serviceInstanceId;
    @SerializedName("service.namespace")
    @Expose
    private String serviceNamespace;
    @SerializedName("telemetry.sdk.language")
    @Expose
    private String telemetrySdkLanguage;
    @SerializedName("telemetry.sdk.name")
    @Expose
    private String telemetrySdkName;
    @SerializedName("telemetry.sdk.version")
    @Expose
    private String telemetrySdkVersion;
    @SerializedName("request_protocol")
    @Expose
    private String requestProtocol;

    public String getCloudZone() {
        return cloudZone;
    }

    public void setCloudZone(String cloudZone) {
        this.cloudZone = cloudZone;
    }

    public String getHostName() {
        return hostName;
    }

    public void setHostName(String hostName) {
        this.hostName = hostName;
    }

    public String getInteractionType() {
        return interactionType;
    }

    public void setInteractionType(String interactionType) {
        this.interactionType = interactionType;
    }

    public String getOtelLibraryName() {
        return otelLibraryName;
    }

    public void setOtelLibraryName(String otelLibraryName) {
        this.otelLibraryName = otelLibraryName;
    }

    public String getOtelLibraryVersion() {
        return otelLibraryVersion;
    }

    public void setOtelLibraryVersion(String otelLibraryVersion) {
        this.otelLibraryVersion = otelLibraryVersion;
    }

    public String getOtelStatusCode() {
        return otelStatusCode;
    }

    public void setOtelStatusCode(String otelStatusCode) {
        this.otelStatusCode = otelStatusCode;
    }

    public String getServiceInstanceId() {
        return serviceInstanceId;
    }

    public void setServiceInstanceId(String serviceInstanceId) {
        this.serviceInstanceId = serviceInstanceId;
    }

    public String getServiceNamespace() {
        return serviceNamespace;
    }

    public void setServiceNamespace(String serviceNamespace) {
        this.serviceNamespace = serviceNamespace;
    }

    public String getTelemetrySdkLanguage() {
        return telemetrySdkLanguage;
    }

    public void setTelemetrySdkLanguage(String telemetrySdkLanguage) {
        this.telemetrySdkLanguage = telemetrySdkLanguage;
    }

    public String getTelemetrySdkName() {
        return telemetrySdkName;
    }

    public void setTelemetrySdkName(String telemetrySdkName) {
        this.telemetrySdkName = telemetrySdkName;
    }

    public String getTelemetrySdkVersion() {
        return telemetrySdkVersion;
    }

    public void setTelemetrySdkVersion(String telemetrySdkVersion) {
        this.telemetrySdkVersion = telemetrySdkVersion;
    }

    public String getRequestProtocol() {
        return requestProtocol;
    }

    public void setRequestProtocol(String requestProtocol) {
        this.requestProtocol = requestProtocol;
    }

}
