package utilityMatrix.recoEval.tools;

import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class Parameters {

   public String baseDir_ = "$HOME/Software/Clustering/";//"$HOME/clustering/";
   public String datasetPath_ = baseDir_ + "/data/data.train";
   public String testingPath_ = baseDir_ + "/data/data.test";
   public String outputPath_ = baseDir_ + "/output/";

   public double trainingFraction_ = 0.8;



   // Not a parameter
   private String[] rawParameters;

   public Parameters(String[] params) {
      rawParameters = params;
      autoAssign(params);
   }

   public Parameters() {
      //Just for having the subclass compliant with my parser
   }

   protected void autoAssign(String[] params) {
      Map<String, String> parameters = readParameters(params);
      for (String param : parameters.keySet()) {
         try {
            String fieldName = getFieldName(param);
            // Throw an exception if the field does not exist.
            Field field = this.getClass().getDeclaredField(fieldName);
            if (!setFieldValue(field, parameters.get(param))) {
               System.err.println("Could not assign parameter: " + param + " " + parameters.get(param));
            } else {
               // System.out.println("Successfully assigned parameter : " +
               // param);// +" "+field.get(this));
               // System.out.println("Successfully assigned parameter : " +
               // param +" "+ parameters.get(param));
            }
         } catch (NoSuchFieldException e) {
            // No field corresponding to the parameter.
            System.err.println("Warning, unknown parameter: " + param);
         } catch (SecurityException e) {
            // Should not happen.
            e.printStackTrace();
         }
      }
   }


   protected boolean setFieldValue(Field field, String value) {
      try {
         field.setBoolean(this, Boolean.parseBoolean(value));
         return true;
      } catch (Exception e) {
         // not bool
      }
      try {
         field.setFloat(this, Float.parseFloat(value));
         return true;
      } catch (Exception e) {
         // not float
      }
      try {
         field.setDouble(this, Double.parseDouble(value));
         return true;
      } catch (Exception e) {
         // not double
      }
      try {
         field.setShort(this, Short.parseShort(value));
         return true;
      } catch (Exception e) {
         // not short
      }
      try {
         field.setInt(this, Integer.parseInt(value));
         return true;
      } catch (Exception e) {
         // not int
      }
      try {
         field.setLong(this, Long.parseLong(value));
         return true;
      } catch (Exception e) {
         // not long
      }
      try {
         field.setByte(this, Byte.parseByte(value));
         return true;
      } catch (Exception e) {
         // not byte
      }
      try {
         field.set(this, ValueList.parseValueList(value));
         return true;
      } catch (Exception e) {
         // not byte
      }
      try {
         field.set(this, StringList.parseStringList(value));
         return true;
      } catch (Exception e) {
         // not byte
      }
      try {
         field.set(this, value);
         return true;
      } catch (Exception e) {
         // not String
      }

      return false;
   }

   public void generateFiles(String out) throws IllegalArgumentException, IllegalAccessException {
      String baseParams = "";

      String generateParams = "params=\"\n";

      for (Field field : this.getClass().getFields()) {
         String fieldName = field.getName();
         String entryName = fieldName.substring(0, fieldName.length() - 1);
         String value = field.get(this).toString();
         baseParams += "export " + entryName + "=" + value + "\n";
         generateParams += entryName + " $" + entryName + "\n";
      }

      generateParams += "\"\n";
      generateParams += "echo $params";

      FilesLoader.logString(out + "exportBaseParameters.sh", baseParams);
      FilesLoader.logString(out + "generateParams.sh", generateParams);

      System.out.println("Created helper scripts exportBaseParameters.sh and generateParams.sh");
   }

   public String getParametersString(String[] params) {
      String str = "";
      try {
         // int assigned = 0, unassigned = 0;
         Map<String, String> parameters = readParameters(params);
         for (Field field : this.getClass().getFields()) {
            String fieldName = field.getName();
            String entryName = fieldName.substring(0, fieldName.length() - 1);
            str += fieldName + " " + field.get(this).toString();
            if (parameters.containsKey(entryName)) {
               str += " assigned ";
               // assigned++;
            } else {
               str += " unassigned ";
               // unassigned++;
               System.out.println("Unassigned: " + fieldName);
            }
            str += "\n";
            if (field.get(this) == null) {
               System.err.println("Warning, the field " + field.getName() + " is not initialised!");
            }
         }
      } catch (Exception e) {
         str = e.toString();
      }
      return str;
   }

   protected static Map<String, String> readParameters(String[] args) {
      Map<String, String> arguments = new HashMap<String, String>();
      try {
         // System.out.println("called with parameter: ");
         for (int i = 0; i < args.length; i++) {
            // System.out.println(args[i] + " " + args[i + 1]);
            arguments.put(args[i], args[i + 1]);
            i++;
         }

      } catch (RuntimeException e) {
         System.err.println("Exception while reading parameters: ");
         for (int i = 0; i < args.length; i++) {
            System.err.print(args[i] + " ");
            System.err.println(args[i + 1]);
            i++;
         }
         throw e;
      }
      return arguments;
   }

   protected static String getFieldName(String param) {
      return param + "_";
   }

   /**
    * Generate helper scripts which ease handling of the simulation parameters in a launcher shell script. Files are
    * generated in the current working directory when running this method.
    *
    * @param toto Optionnal values to replace the default parameters
    */
   public static void main(String[] toto) throws IllegalArgumentException, IllegalAccessException {
      Parameters params = new Parameters(toto);
      params.generateFiles("./launcher/scripts/");
   }

   @Override
   public String toString() {
      return "Parameters{" +
            "baseDir_='" + baseDir_ + '\'' +
            ", datasetPath_='" + datasetPath_ + '\'' +
            ", testingPath_='" + testingPath_ + '\'' +
            ", outputPath_='" + outputPath_ + '\'' +
            ", trainingFraction_=" + trainingFraction_ +
            ", rawParameters=" + Arrays.toString(rawParameters) +
            '}';
   }
}
