package utilityMatrix.recoEval.tools;

/**
 * Contains a steadyStateList of string values. Can be created with either a single string, or a steadyStateList of
 * string in the form : {v1,v2,v3,v4}
 *
 * @author ajegou
 */
public class StringList {

   private final String[] values_;

   private StringList(String[] values) {
      values_ = values;
   }

   public static StringList emptyList() {
      return new StringList(new String[0]);
   }

   public static StringList parseStringList(String values) {
      values = values.replaceAll(" ", "");
      if (values.length() == 0 || values.equals("{}")) {
         return new StringList(new String[0]);
      } else if (values.startsWith("{")) {
         String content = values.substring(1, values.length() - 1);
         // Debug
         System.out.println("debug: content = " + content);
         String[] parameters = content.split(",");
         return new StringList(parameters);
      } else {
         return new StringList(new String[]{values});
      }
   }

   public String[] getValues() {
      return values_;
   }

   @Override
   public String toString() {
      if (values_.length == 0) {
         return "{}";
      }
      String str = "{";

      str += values_[0];
      for (int i = 1; i < values_.length; i++) {
         str += "," + values_[i];
      }
      str += "}";
      return str;
   }
}
