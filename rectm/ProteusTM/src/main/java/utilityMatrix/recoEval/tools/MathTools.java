package utilityMatrix.recoEval.tools;

import org.apache.commons.math3.stat.regression.SimpleRegression;
import runtime.Pair;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 25/07/14
 */
public class MathTools {

   /**
    * @param numIndices
    * @param total
    * @param seed
    * @return
    */
   public static List<Integer> drawIndicesUniformly(int numIndices, int total, long seed) {
      //define ArrayList to hold Integer objects
      final ArrayList<Integer> numbers = new ArrayList<>();
      for (int i = 0; i < total; i++) {
         numbers.add(i);
      }
      Collections.shuffle(numbers, new Random(seed));
      return numbers.subList(0, numIndices);
   }

   public static List<Integer> drawIndicesUniformly(int numIndices, int init, int end, Random random) {
      //define ArrayList to hold Integer objects
      final ArrayList<Integer> numbers = new ArrayList<>();
      for (int i = init; i <= end; i++) {
         numbers.add(i);
      }
      Collections.shuffle(numbers, random);
      return numbers.subList(0, numIndices);
   }

   public static List<Integer> drawUniformlyExcluding(int numIndicesToDraw, int init, int end, Random random, List<Integer> fixed) {
      //define ArrayList to hold Integer objects
      final ArrayList<Integer> numbers = new ArrayList<>();
      for (int i = init; i <= end; i++) {
         if (!fixed.contains(i)) {
            numbers.add(i);
         }
      }
      Collections.shuffle(numbers, random);
      return numbers.subList(0, numIndicesToDraw);
   }

   public static int countRowsInFile(File f, boolean countHeader) {
      try {
         int totalSize = 0;
         BufferedReader br = new BufferedReader(new FileReader(f));
         while ((br.readLine() != null)) {
            totalSize++;
         }
         br.close();
         return countHeader ? totalSize : (totalSize - 1);
      } catch (IOException e) {
         throw new RuntimeException(e);
      }
   }

   public static int countRowsInFile(String f, boolean countHeader) {
      try {
         int totalSize = 0;
         BufferedReader br = new BufferedReader(new FileReader(f));
         while ((br.readLine() != null)) {
            totalSize++;
         }
         br.close();
         return countHeader ? totalSize : (totalSize - 1);
      } catch (IOException e) {
         throw new RuntimeException(e);
      }
   }

   public static int columnPerRow(String srcFile, String SEPARATOR, boolean countFirstColumn) {
      try {
         BufferedReader reader = new BufferedReader(new FileReader(srcFile));
         String line = reader.readLine();
         final int total = line.split(SEPARATOR).length;
         reader.close();
         if (countFirstColumn)
            return total;
         return total - 1;
      } catch (IOException e) {
         throw new RuntimeException(e);
      }
   }

   public static double average(Double[] array) {
      double sum = 0;
      for (double d : array) {
         sum += d;
      }
      return sum / ((double) array.length);
   }


   public static double variance(boolean normalizeWrtMean, Double... array) {
      double sum = 0;
      final double mean = average(array);
      for (double d : array) {
         sum += (d - mean) * (d - mean);
      }
      sum = sum / ((double) array.length);
      if (normalizeWrtMean) {
         sum = sum / mean;
      }
      return sum;
   }

   /**
    * Compute the mean value in the array and return the highest quadratic difference among the values of the array and
    * the mean
    *
    * @param array
    * @return
    */
   public static double highestQuadraticDifferenceFromMean(boolean normalizeWrtMean, Double... array) {
      double maxDiff = 0;
      final double mean = average(array);
      for (double d : array) {
         double currDiff = (d - mean) * (d - mean);
         if (currDiff > maxDiff) {
            maxDiff = currDiff;
         }
      }
      if (normalizeWrtMean) {  //Normalization wrt to a constant can be performed at the end
         maxDiff = maxDiff / (mean * mean);
      }
      return maxDiff;
   }


   public static double relErr(double real, double pred) {
      if (real == pred) {
         return 0;
      }
      if (real == 0) {
         return 1;
      }
      return Math.abs(real - pred) / real;
   }

   /**
    * Signed relative error (it preserves the sign of the error)
    *
    * @param real
    * @param pred
    * @return
    */
   public static double signedRelErr(double real, double pred) {
      if (real == pred) {
         return 0;
      }
      if (real == 0) {
         return 1;
      }
      return (real - pred) / Math.abs(real);
   }


   public static double MAPE(List<Pair<Double, Double>> pairs) {
      double sum = 0;
      for (Pair<Double, Double> pair : pairs) {
         sum += relErr(pair.getFirst(), pair.getSecond());
      }
      return sum / pairs.size();
   }

   /**
    * http://en.wikipedia.org/wiki/Root-mean-square_deviation
    *
    * @param pairs
    * @return
    */
   public static double RMSE(List<Pair<Double, Double>> pairs) {
      double sum = 0;
      for (Pair<Double, Double> pair : pairs) {
         sum += Math.pow(pair.getFirst() - pair.getSecond(), 2D);
      }
      return Math.sqrt(sum / pairs.size());
   }

   /**
    * http://en.wikipedia.org/wiki/Correlation_and_dependence
    *
    * @param pairs
    * @return
    */
   public static double CORR(List<Pair<Double, Double>> pairs) {
      Double[] reals = new Double[pairs.size()], preds = new Double[pairs.size()];
      double meanReal, meanPred;
      int i = 0;
      for (Pair<Double, Double> p : pairs) {
         reals[i] = p.getFirst();
         preds[i++] = p.getSecond();
      }
      meanReal = average(reals);
      meanPred = average(preds);
      double numerator = 0;
      for (i = 0; i < pairs.size(); i++) {
         numerator += (pairs.get(i).getFirst() - meanReal) * (pairs.get(i).getSecond() - meanPred);
      }
      double squaredSumReal = 0;
      for (Pair<Double, Double> pair : pairs) {
         squaredSumReal += Math.pow(pair.getFirst() - meanReal, 2D);
      }
      double squaredSumPred = 0;
      for (Pair<Double, Double> pair : pairs) {
         squaredSumPred += Math.pow(pair.getSecond() - meanPred, 2D);
      }

      return numerator / Math.sqrt(squaredSumPred * squaredSumReal);

   }

   /**
    * http://en.wikipedia.org/wiki/Coefficient_of_determination
    *
    * @param pairs
    * @return
    */
   @Deprecated
   public static double wikiRSQ(List<Pair<Double, Double>> pairs) {
      Double[] reals = new Double[pairs.size()];
      int i = 0;
      for (Pair<Double, Double> p : pairs) {
         reals[i++] = p.getFirst();
      }
      double meanReal = average(reals);
      double squaredSumReal = 0;
      for (Pair<Double, Double> pair : pairs) {
         squaredSumReal += Math.pow(pair.getFirst() - meanReal, 2D);
      }
      double squaredSumPred = 0;
      for (Pair<Double, Double> pair : pairs) {
         squaredSumPred += Math.pow(pair.getFirst() - pair.getSecond(), 2D);
      }
      return 1.D - (squaredSumPred / squaredSumReal);
   }

   /**
    * http://www.r-tutor.com/elementary-statistics/simple-linear-regression/coefficient-determination
    *
    * @param pairs
    * @return
    */
   public static double RRSQ(List<Pair<Double, Double>> pairs) {
      Double[] reals = new Double[pairs.size()];
      int i = 0;
      for (Pair<Double, Double> pair : pairs) {
         reals[i++] = pair.getFirst();
      }
      double meanReal = average(reals);
      double squaredSumReal = 0;
      for (Pair<Double, Double> pair : pairs) {
         squaredSumReal += Math.pow(pair.getFirst() - meanReal, 2D);
      }
      double squaredSumPred = 0;
      for (Pair<Double, Double> pair : pairs) {
         squaredSumPred += Math.pow(pair.getSecond() - meanReal, 2D);
      }
      return (squaredSumPred / squaredSumReal);
   }


   public static double apacheRSQ(List<Pair<Double, Double>> pairs) {
      double[][] reals = new double[pairs.size()][1];
      int i = 0;
      for (Pair<Double, Double> pair : pairs) {
         reals[i++][0] = pair.getFirst();
      }
      double[] preds = new double[pairs.size()];
      i = 0;
      for (Pair<Double, Double> pair : pairs) {
         preds[i++] = pair.getSecond();
      }
      SimpleRegression r = new SimpleRegression();
      r.addObservations(reals, preds);
      r.regress();
      return r.getRSquare();
   }

   public static double apacheCORR(List<Pair<Double, Double>> pairs) {
      double[][] reals = new double[pairs.size()][1];
      int i = 0;
      for (Pair<Double, Double> pair : pairs) {
         reals[i++][0] = pair.getFirst();
      }
      double[] preds = new double[pairs.size()];
      i = 0;
      for (Pair<Double, Double> pair : pairs) {
         preds[i++] = pair.getSecond();
      }
      SimpleRegression r = new SimpleRegression();
      r.addObservations(reals, preds);
      r.regress();
      return r.getR();
   }


   public static long factorial(int i) {
      if (i < 0)
         throw new IllegalArgumentException(i + " is less than 0");
      if (i == 0 || i == 1)
         return 1;
      return i * factorial(i - 1);
   }

   public static long binomial(int n, int k) {
      return factorial(n) / (factorial(k) * factorial(n - k));
   }

}
