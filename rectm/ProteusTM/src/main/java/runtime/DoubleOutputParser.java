package runtime;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 03/09/14
 */
public class DoubleOutputParser extends OutputParser<Double> {

   public DoubleOutputParser(String filePath, String sep) throws IOException {
      super(filePath, sep);
   }

   @Override
   protected Double _getParam(String o) {
      return Double.parseDouble(o);
   }


   public double avg(String param) throws ParameterNotFoundException {
      final List<Double> doubles = column(param);
      double avg = 0D;
      for (Double d : doubles) {
         avg += d;
      }
      return avg / (double) doubles.size();
   }

   public double min(String param) throws ParameterNotFoundException {
      final List<Double> doubles = column(param);
      double min = Double.MAX_VALUE;
      for (Double d : doubles) {
         if (d < min) {
            min = d;
         }
      }
      return min;
   }

   public double max(String param) throws ParameterNotFoundException {
      final List<Double> doubles = column(param);
      double max = Double.MIN_VALUE;
      for (Double d : doubles) {
         if (d > max) {
            max = d;
         }
      }
      return max;
   }

   /**
    * Return the average value for row "row" among the values at columns given by "params"
    *
    * @param row
    * @param params
    * @return
    */
   private double avgAmong(int row, String... params) throws ParameterNotFoundException {
      double avg = 0D;
      for (String s : params) {
         avg += getParam(s, row);
      }
      avg /= params.length;
      return avg;
   }

   private double maxAmong(int row, String... params) throws ParameterNotFoundException {
      double max = Double.MIN_VALUE;
      double temp;
      for (String s : params) {
         temp = getParam(s, row);
         if (temp > max)
            max = temp;
      }
      return max;
   }

   private double minAmong(int row, String... params) throws ParameterNotFoundException {
      double min = Double.MAX_VALUE;
      double temp;
      for (String s : params) {
         temp = getParam(s, row);
         if (temp < min)
            min = temp;
      }
      return min;
   }


   public double minAmong(String idParam, String row, String... params) throws ParameterNotFoundException {
      int rowId = idWithStringColumn(idParam, row);
      return minAmong(rowId, params);

   }


   /**
    * Find the max in a row which is identified by the id "row" for the parameter "idParam", considering only the
    * columns params
    *
    * @param idParam
    * @param row
    * @param params
    * @return
    * @throws ParameterNotFoundException
    */
   public double maxAmong(String idParam, String row, String... params) throws ParameterNotFoundException {

      int rowId = idWithStringColumn(idParam, row);
      return maxAmong(rowId, params);

   }

   public String argMaxAmong(String idParam, String row, String... params) throws ParameterNotFoundException {

      int rowId = idWithStringColumn(idParam, row);
      return argMaxAmong(rowId, params);

   }

   private String argMaxAmong(int row, String... params) throws ParameterNotFoundException {
      double max = Double.MIN_VALUE;
      List<String> candidates = new ArrayList<>();
      double temp;
      for (String s : params) {
         temp = getParam(s, row);
         if (temp > max) {
            candidates.clear();
            max = temp;
            candidates.add(s);
         } else if (temp == max) {
            candidates.add(s);
         }
      }
      if (candidates.size() > 1) {
         System.out.println(idFor(row) + " More than one argmax found " + candidates);
      }
      return candidates.get(0);
   }

   public String idFor(int row) {
      try {
         return rawColumn("Benchmark").get(row);
      } catch (ParameterNotFoundException e) {
         e.printStackTrace();  // TODO: Customise this generated block
         return null;
      }
   }

   public double meanAmong(String idParam, String row, String... params) throws ParameterNotFoundException {

      int rowId = idWithStringColumn(idParam, row);
      return avgAmong(rowId, params);

   }

   public Double[] minsByRow() {
      Double[] mins = new Double[numRows()];
      List<String> allConfigs = allParams();
      allConfigs.remove("Benchmark");
      String[] configs = allConfigs.toArray(new String[allConfigs.size()]);
      for (int i = 0; i < numRows(); i++) {
         try {
            mins[i] = minAmong(i, configs);
         } catch (ParameterNotFoundException e) {
            throw new RuntimeException(e);
         }
      }
      return mins;
   }

   public Double[] maxesByRow() {
      Double[] maxes = new Double[numRows()];
      List<String> allConfigs = allParams();
      allConfigs.remove("Benchmark");
      String[] configs = allConfigs.toArray(new String[allConfigs.size()]);
      for (int i = 0; i < numRows(); i++) {
         try {
            maxes[i] = maxAmong(i, configs);
         } catch (ParameterNotFoundException e) {
            throw new RuntimeException(e);
         }
      }
      return maxes;
   }

   private double maxFor(List<Double> doubles) {
      double max = Double.MIN_VALUE;
      for (Double d : doubles) {
         if (d > max) {
            max = d;
         }
      }
      return max;
   }

   private double minFor(List<Double> doubles) {
      double min = Double.MAX_VALUE;
      for (Double d : doubles) {
         if (d < min) {
            min = d;
         }
      }
      return min;
   }


   public double max(int paramIndex) {
      final List<Double> doubles = column(paramIndex);
      return maxFor(doubles);
   }

   /**
    * Max value in a row
    *
    * @param rowId
    * @return
    */
   public double maxInRow(int rowId) {
      try {
         return maxAmong(rowId, headerExcluding("Benchmark"));
      } catch (ParameterNotFoundException e) {
         throw new RuntimeException(e);
      }
   }

   /**
    * Find the overallMax
    *
    * @return
    */
   public double overallMax() {
      Double[] maxes = maxesByRow();
      return maxFor(Arrays.asList(maxes));
   }


   public double overAllMin() {
      Double[] mins = minsByRow();
      return minFor(Arrays.asList(mins));
   }

}
