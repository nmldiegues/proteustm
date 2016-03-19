package runtime;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.Arrays;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 01/03/15
 */
public class ProteusTMIntegerToRuntimeConfig implements IntegerToRuntimeConfig {


   public static void main(String args[]) {
      test();
   }


   private static void test() {
      ProteusTMIntegerToRuntimeConfig recTMIntegerToRuntimeConfig = new ProteusTMIntegerToRuntimeConfig();
      //String ret = recTMIntegerToRuntimeConfig.fromIntToRuntimeConfig("greentm_stm-norec=1th=MCS_LOCKS");
      //System.out.println(ret);
      String ret = recTMIntegerToRuntimeConfig.fromIntToRuntimeConfig("greentm_htm-sgl=LINEAR_DECREASE=RETRY_BUDGET_0=1");
      System.out.println(ret);
      //ret = recTMIntegerToRuntimeConfig.fromIntToRuntimeConfig("greentm_htm-sgl=LINEAR_DECREASE=RETRY_BUDGET_20=8");
      //System.out.println(ret);

   }

   private final static Log log = LogFactory.getLog(IntegerToRuntimeConfig.class);
   final static boolean t = log.isTraceEnabled();
   /**
    * greentm_stm-norec=1th=MCS_LOCKS greentm_htm-sgl=HALF_DECREASE=RETRY_BUDGET_20=5
    *
    * @param toParse
    * @return
    */
   @Override
   public String fromIntToRuntimeConfig(String toParse) {

      if (t) log.trace("Suggested string config " + toParse);
      String[] parsed = toParse.split("=");
      if (t) log.trace("Input parsed " + Arrays.toString(parsed));
      String backend = null, thread = null, retryPolicy = null, retryBudget = null;
      if (isSTM(parsed[0])) {
         if (t)log.trace("Parsing stm");
         backend = extractSTM(parsed[0]);
         if (t)log.trace("STM is " + backend);
         thread = stmThread(parsed[1]);
         retryPolicy = "0";
         retryBudget = "0";
      } else {
         if (isHybrid(parsed[0])) {
            if(t)log.trace("parsing hybrid");
            backend = hytm(parsed[0]);
         } else {
            if (t)log.trace("parsing htm");
            backend = "htm";
         }
         retryPolicy = retryPolicy(parsed[1]);
         retryBudget = retryBudget(parsed[2]);
         thread = parsed[3];

      }

      String toRet = backend + "=" + thread + "=" + retryPolicy + "=" + retryBudget;
      if (log.isInfoEnabled()) log.info("Returning " + toRet);
      return toRet.replace("=", "\n");
   }

   private String extractSTM(String s) {
      return s.substring("greentm_stm-".length(), s.length());
   }

   private String hytm(String s) {
      if (s.contains("norec"))
         return "hybrid-norec-ptr";
      return "hybrid-tl2-ptr";
   }

   private String stmThread(String t) {
      String[] parsed = t.split("th");
      return parsed[0];
   }

   private String retryBudget(String r) {
      String[] parsed = r.split("_");
      return parsed[2];
   }

   private String retryPolicy(String r) {
      if (r.equals("LINEAR_DECREASE"))
         return IntegerToRuntimeConfig.LINEAR;
      if (r.equals("HALF_DECREASE"))
         return IntegerToRuntimeConfig.HALF;
      return IntegerToRuntimeConfig.GIVEUP;
   }

   private boolean isSTM(String s) {
      return (!s.contains("hybrid")) && (s.contains("norec") || s.contains("tinystm") || s.contains("tl2") || s.contains("swisstm"));
   }

   private boolean isHybrid(String s) {
      return s.contains("hybrid");
   }
}
