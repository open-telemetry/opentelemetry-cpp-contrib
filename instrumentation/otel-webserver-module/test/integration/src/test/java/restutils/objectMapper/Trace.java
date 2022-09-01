package restutils.objectMapper;

import com.google.gson.annotations.Expose;
import com.google.gson.annotations.SerializedName;

public class Trace {

    @SerializedName("traceId")
    @Expose
    private String traceId;
    @SerializedName("parentId")
    @Expose
    private String parentId;
    @SerializedName("id")
    @Expose
    private String id;
    @SerializedName("kind")
    @Expose
    private String kind;
    @SerializedName("name")
    @Expose
    private String name;
    @SerializedName("timestamp")
    @Expose
    private Long timestamp;
    @SerializedName("duration")
    @Expose
    private Integer duration;
    @SerializedName("localEndpoint")
    @Expose
    private LocalEndpoint localEndpoint;
    @SerializedName("tags")
    @Expose
    private Tags tags;

    public String getTraceId() {
        return traceId;
    }

    public void setTraceId(String traceId) {
        this.traceId = traceId;
    }

    public String getParentId() {
        return parentId;
    }

    public void setParentId(String parentId) {
        this.parentId = parentId;
    }

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public String getKind() {
        return kind;
    }

    public void setKind(String kind) {
        this.kind = kind;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Long getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Long timestamp) {
        this.timestamp = timestamp;
    }

    public Integer getDuration() {
        return duration;
    }

    public void setDuration(Integer duration) {
        this.duration = duration;
    }

    public LocalEndpoint getLocalEndpoint() {
        return localEndpoint;
    }

    public void setLocalEndpoint(LocalEndpoint localEndpoint) {
        this.localEndpoint = localEndpoint;
    }

    public Tags getTags() {
        return tags;
    }

    public void setTags(Tags tags) {
        this.tags = tags;
    }

}
