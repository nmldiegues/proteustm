package utilityMatrix.recoEval.datasetLoader.normalization;



import runtime.DoubleIntPair;
import runtime.DoubleOutputParser;
import runtime.MatrixHelper;

import java.util.TreeSet;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 19/10/14 This class is aimed at finding the normalization such that the variance on the max value is minimal
 * (i.e., the "scale" is as much as possible uniform)
 */
public class MinVarianceOnMaxFinder {
   private static final String temp_file = "ProteusTM/data/temp.csv";

   private static boolean _USE_MAX = false;


   public static void setUsingMax() {
      _USE_MAX = true;
   }

   private String originalFile;
   private int numConfigs;

   public MinVarianceOnMaxFinder(String originalFile, int numConfigs) {
      this.originalFile = originalFile;
      this.numConfigs = numConfigs;
      System.out.println(_USE_MAX ? "Reasoning on max" : "Reasoning on min");
   }

   public DoubleIntPair lowestVar() {
      MinVarianceOnMaxFinder minVarianceOnMaxFinder = new MinVarianceOnMaxFinder(originalFile, numConfigs);
      TreeSet<DoubleIntPair> set = minVarianceOnMaxFinder.printStats();
      return set.first();
   }

   public DoubleIntPair highestVar() {
      MinVarianceOnMaxFinder minVarianceOnMaxFinder = new MinVarianceOnMaxFinder(originalFile, numConfigs);
      TreeSet<DoubleIntPair> set = minVarianceOnMaxFinder.printStats();
      return set.last();
   }

   public TreeSet<DoubleIntPair> printStats() {
      TreeSet<DoubleIntPair> pairs = new TreeSet<>();
      for (int i = 1; i <= numConfigs; i++) {
         DoubleIntPair pair = statsFor(i);
         if (pair != null) {
            pairs.add(pair);
         } else {
            break;
         }
      }
      //System.out.println(pairs);
      return pairs;
   }

   private DoubleIntPair statsFor(int columnIndex) {
      try {
         MatrixHelper.normalizeWrt(originalFile, temp_file, columnIndex, ",");
         DoubleOutputParser dop = new DoubleOutputParser(temp_file, ",");
         info inf = computeInfo(dop);
         //System.out.println(columnIndex + "," + dop.columnId(columnIndex) + "," + inf.min + "," + inf.max + "," + inf.mean + "," + inf.var + "," + inf.normVar);
         return new DoubleIntPair(inf.normVar, columnIndex);
      } catch (Exception e) {
         if (e instanceof ArrayIndexOutOfBoundsException)
            return null;
         throw new RuntimeException(e);
      }
   }


   private info computeInfo(DoubleOutputParser dop) {
      if (_USE_MAX)
         return computeMaxInfo(dop);
      return computeMinInfo(dop);
   }

   private info computeMaxInfo(DoubleOutputParser dop) {
      Double[] maxes = dop.maxesByRow();
      //System.out.println(Arrays.toString(maxes));
      double minMax = min(maxes);
      double maxMax = max(maxes);
      double meanMax = avg(maxes);
      double varMax = variance(maxes, meanMax);
      double normVarMax = varMax / meanMax;
      return new info(minMax, maxMax, meanMax, varMax, normVarMax);
   }

   private info computeMinInfo(DoubleOutputParser dop) {
      Double[] mins = dop.minsByRow();
      double minMin = min(mins);
      double maxMin = max(mins);
      double meanMin = avg(mins);
      double varMin = variance(mins, meanMin);
      double normVarMin = varMin / meanMin;
      return new info(minMin, maxMin, meanMin, varMin, normVarMin);
   }

   private static double avg(Double[] doubles) {
      double sum = 0;
      for (double d : doubles) {
         sum += d;
      }
      return sum / doubles.length;
   }

   private static double min(Double[] doubles) {
      double min = doubles[0];
      for (double d : doubles) {
         if (d < min) {
            min = d;
         }
      }
      return min;
   }

   private static double max(Double[] doubles) {
      double max = doubles[0];
      for (double d : doubles) {
         if (d > max) {
            max = d;
         }
      }
      return max;
   }

   private static double variance(Double[] doubles) {
      double avg = avg(doubles);
      return variance(doubles, avg);
   }

   private static double variance(Double[] doubles, double mean) {
      double sum = 0;
      for (double d : doubles) {
         sum += (d - mean) * (d - mean);
      }
      return sum / doubles.length;
   }

   private class info {
      double min, max, mean, var, normVar;

      private info(double min, double max, double mean, double var, double normVar) {
         this.min = min;
         this.max = max;
         this.mean = mean;
         this.var = var;
         this.normVar = normVar;
      }
   }

}
