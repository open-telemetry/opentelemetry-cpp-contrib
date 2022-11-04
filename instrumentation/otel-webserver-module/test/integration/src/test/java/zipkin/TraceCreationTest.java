package zipkin;

import org.testng.annotations.Test;
import restutils.BaseTest;
import restutils.ValidationUtils;

public class TraceCreationTest extends BaseTest {
    ValidationUtils validationUtils = new ValidationUtils();

    @Test
    public void checkServiceName() throws Exception {
        validationUtils.verifyAllServices();

    }

    @Test
    public void checkSpans() throws Exception {
        validationUtils.verifyAllSpans();
    }

    @Test()
    public void testTraceAndSpansCreation() throws Exception {
        validationUtils.verifyAllTraces();
    }

}