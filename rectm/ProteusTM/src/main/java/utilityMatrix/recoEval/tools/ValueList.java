package utilityMatrix.recoEval.tools;

import gnu.trove.list.TDoubleList;
import gnu.trove.list.array.TDoubleArrayList;

import java.math.BigDecimal;

/**
 * Contains a steadyStateList of double values. Can be created with a steadyStateList of double values, an interval of
 * values and a step, or a string representation. The string representation is either {v1,v2,v3,v4} when the string
 * contains a steadyStateList of values, or [begin,end,step] when the string contains an interval.
 *
 * @author ajegou
 */
public class ValueList {

   private final double[] values_;

   private ValueList(double[] values) {
      values_ = values;
   }

   public static ValueList emptyList() {
      return new ValueList(new double[0]);
   }

   public static ValueList buildValueList(BigDecimal begin, BigDecimal end, BigDecimal step) {
      TDoubleList list = new TDoubleArrayList();

      BigDecimal current = begin;
      while (current.compareTo(end) <= 0) {
         list.add(current.doubleValue());
         current = current.add(step);
      }
      return new ValueList(list.toArray());
   }

   public static ValueList parseValueList(String values) {
      values = values.replaceAll(" ", "");
      if (values.length() == 0 || values.equals("{}")) {
         return new ValueList(new double[0]);
      } else if (values.startsWith("[")) {
         String content = values.substring(1, values.length() - 1);
         String[] parameters = content.split(",");
         BigDecimal begin = new BigDecimal(parameters[0]);
         BigDecimal end = new BigDecimal(parameters[1]);
         BigDecimal step = new BigDecimal(parameters[2]);
         return buildValueList(begin, end, step);
      } else if (values.startsWith("{")) {
         String content = values.substring(1, values.length() - 1);
         // Debug
         System.out.println("debug: content = " + content);
         String[] parameters = content.split(",");
         double[] doubleValues = new double[parameters.length];
         for (int index = 0; index < doubleValues.length; index++) {
            doubleValues[index] = Double.parseDouble(parameters[index]);
         }
         return new ValueList(doubleValues);
      } else {
         return new ValueList(new double[]{Double.parseDouble(values)});
      }
   }

   public double[] getValues() {
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
