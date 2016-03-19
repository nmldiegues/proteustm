package utilityMatrix.recoEval.datasetLoader.normalization;

import runtime.DoubleOutputParser;
import utilityMatrix.recoEval.tools.ExtendedParameters;
import utilityMatrix.recoEval.tools.MathTools;
import xml.DXmlParser;

import java.io.IOException;
import java.util.Arrays;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 22/10/14
 */
public class Normalizator {

   private normalization norm;
   private double wrt;
   private ExtendedParameters ep;
   private DoubleOutputParser doubleOutputParser;
   private final boolean forceNonNegative = true;
   private Double[] cachedWRT;
   private Double[] cacheWRT_2;

   public double getWrt() {
      return wrt;
   }

   @Override
   public String toString() {
      return "Normalizator{" +
            "norm=" + norm +
            ", wrt=" + wrt +
            ", ep=" + ep +
            ", doubleOutputParser=" + doubleOutputParser +
            ", forceNonNegative=" + forceNonNegative +
            ", cachedWRT=" + Arrays.toString(cachedWRT) +
            ", cacheWRT_2=" + Arrays.toString(cacheWRT_2) +
            '}';
   }

   public Normalizator(normalization norm, String file, double wrt, ExtendedParameters extendedParameters) {
      this.norm = norm;
      this.ep = extendedParameters;
      this.wrt = wrt;
      if (file != null) {
         try {
            this.doubleOutputParser = new DoubleOutputParser(file, ",");
            if (norm.equals(normalization.MAX)) {
               cachedWRT = overallMaxes();
            } else if (norm.equals(normalization.WRT_REF) || norm.equals(normalization.LOG_10_WRT_REF) || norm.equals(normalization.WRT_REF_LOG_10)) {
               cachedWRT = wrtPerRow();
            } else if (norm.equals(normalization.WRT_BEST)) {
               this.cachedWRT = this.doubleOutputParser.maxesByRow();
            } else if (norm.equals(normalization.WRT_NORM_BEST)) {
               this.cachedWRT = this.doubleOutputParser.minsByRow();
               this.cacheWRT_2 = this.doubleOutputParser.maxesByRow();
            }
         } catch (IOException e) {
            throw new RuntimeException(e);
         }
      }
   }


   /**
    * Produce normalization
    *
    * @param user
    * @param item
    * @param score
    * @return
    */
   public double normalize(int user, int item, double score) {
      if (!ep.isUsingOnlyRatings() && item > ep.getLastRatingColumn()) {
         return score;
      }
      switch (norm) {
         case NONE: {
            return score;
         }
         case LOG_10: {
            return forceNonNegative ? Math.log10(1 + score) : Math.log10(score);
         }
         case WRT_BEST: {
            double bestForUser = this.doubleOutputParser.maxInRow(user);
            return score / bestForUser;
         }
         case WRT_REF: {
            double valueFor = this.cachedWRT[user];
            return score / valueFor;
         }
         case FIXED: {
            return score / wrt;
         }
         case MAX: {
            return score / this.cachedWRT[user];
         }
         case WRT_REF_LOG_10: {
            double valueFor = this.cachedWRT[user];
            double norm = score / valueFor;
            return forceNonNegative ? Math.log10(1 + norm) : Math.log10(norm);
         }
         case LOG_10_WRT_REF: {
            double norm = forceNonNegative ? Math.log10(1 + score) : Math.log10(score);
            //System.out.print(score + " " + norm + " ");
            double valueFor = this.cachedWRT[user];
            //System.out.print(valueFor + " ");
            //I have to take the log of the value, in order to divide by it when it is normalized!!
            valueFor = forceNonNegative ? Math.log10(1 + valueFor) : Math.log10(valueFor);
            return norm / valueFor;
            //System.out.println(valueFor + " " + norm);
         }
         case RECIPROCAL: {
            return 1.0D / score;
         }
         case WRT_NORM_BEST: {
            double min = cachedWRT[user];
            double max = cacheWRT_2[user];
            double ret = (score - min) / (max - min);
            if (ret < 0) {
               throw new RuntimeException(item + ", " + user + " gives a negative normalized value with WRT_NORM_BEST");
            }
            return ret;
         }
         default: {
            throw new IllegalArgumentException("Normalization not recognized");
         }
      }
   }

   public double denormalize(int user, int item, double score) {
      if (!ep.isUsingOnlyRatings() && item > ep.getLastRatingColumn()) {
         return score;
      }
      switch (norm) {
         case NONE: {
            return score;
         }
         case LOG_10: {
            return forceNonNegative ? Math.pow(10, score) - 1D : Math.pow(10, score);
         }
         case WRT_BEST: {
            double bestForUser = this.cachedWRT[user];
            return score * bestForUser;
         }
         case WRT_REF: {
            double valueFor = this.cachedWRT[user];
            return score * valueFor;
         }
         case FIXED: {
            return score * wrt;
         }
         case MAX: {
            return score * this.cachedWRT[user];
         }
         //If you have first wrt then log, now first de-log then de-wrt
         case WRT_REF_LOG_10: {
            double norm = forceNonNegative ? Math.pow(10, score) - 1 : Math.pow(10, score);
            double valueFor = this.cachedWRT[user];
            return norm * valueFor;
         }
         //If you have first Log then wrt, now first de-wrt then de-log
         case LOG_10_WRT_REF: {
            double valueFor = this.cachedWRT[user];
            //System.out.print(score + " " + valueFor + " ");
            //I have to recover the original normalization w.r.t. conf, before multiplying
            valueFor = forceNonNegative ? Math.log10(valueFor + 1) : Math.log10(valueFor);
            //System.out.print(valueFor + " ");
            double norm = score * valueFor;
            //System.out.print(norm + " ");
            return forceNonNegative ? Math.pow(10, norm) - 1 : Math.pow(10, norm);
            //System.out.println(norm);
         }
         case RECIPROCAL: {
            return 1.0D / score;
         }
         case WRT_NORM_BEST: {
            double min = cachedWRT[user];
            double max = cacheWRT_2[user];
            return (score * (max - min)) + min;
         }
         default: {
            throw new IllegalArgumentException("Normalization not recognized");
         }
      }
   }

   private double log10(double value, boolean forceNonNegative) {
      return Math.log10(value + (forceNonNegative ? 1 : 0));
   }

   private double delog10(double value, boolean forceNonNegative) {
      return Math.pow(10, value) - (forceNonNegative ? 1D : 0);
   }


   /**
    * Find the overall max and create an array containing it (one row per user)
    *
    * @return
    */
   private Double[] overallMaxes() {
      double max = this.doubleOutputParser.overallMax();
      int rows = this.doubleOutputParser.numRows();
      Double[] ret = new Double[rows];
      for (int i = 0; i < rows; i++) {
         ret[i] = max;
      }
      return ret;
   }

   /**
    * Create an array with the values of the <user, wrt>, where wrt is the config we are normalizing against
    *
    * @return
    */
   private Double[] wrtPerRow() {
      int rows = this.doubleOutputParser.numRows();
      Double[] ret = new Double[rows];
      int wrt = (int) this.wrt;
      for (int user = 0; user < rows; user++) {
         double valueFor = this.doubleOutputParser.getAt(user, wrt);
         ret[user] = valueFor;
      }
      return ret;
   }

   public enum normalization {
      NONE, LOG_10, WRT_BEST, WRT_NORM_BEST, WRT_REF, FIXED, MAX, LOG_10_WRT_REF, WRT_REF_LOG_10, RECIPROCAL
   }


   public static Normalizator buildNormalizator(ExtendedParameters ep) {
      normalization normalization = ep.getDataNormalization();
      double wrt;
      if (ep.isNormalizingWrtConstant()) {
         wrt = Double.parseDouble(ep.getNormalizeWRT());
      } else if (ep.isNormalizingWrtConf()) {
         try {
            wrt = new DoubleOutputParser(ep.getFullData(), ",").idFor(ep.getNormalizeWRT());
         } catch (IOException e) {
            throw new RuntimeException(e);
         }
      } else {
         wrt = 0;
      }
      String file = ep.getFullData();
      return new Normalizator(normalization, file, wrt, ep);
   }

   public static Normalizator buildFixedRuntimeNormalizator(double wrt, ExtendedParameters ep) {
      return new Normalizator(normalization.FIXED, null, wrt, ep);
   }

   /**
    * Given a column id, returns the corresponding config
    *
    * @param ep
    * @return
    */
   private int normalizationID(ExtendedParameters ep) {
      try {
         return Integer.parseInt(ep.getNormalizeWRT());
      } catch (NumberFormatException n) {
         DoubleOutputParser dop = null;
         try {
            dop = new DoubleOutputParser(ep.getFullData(), ",");
         } catch (IOException e) {
            throw new RuntimeException(e);
         }
         return dop.idFor(ep.getNormalizeWRT());
      }
   }


   private final static String eps = "UtilityMatrix/conf/diegoParams.xml";
   private final static String _OUT_ = "UtilityMatrix/data/testMult/out.csv";

   public static void main(String[] args) throws Exception {
      ExtendedParameters ep = new DXmlParser<ExtendedParameters>().parse(eps);
      ep.setFullData(_OUT_);
      ep.setNormalizeWRT("C_10");
      double costant = 1e3;
      String nonConstantWRT = ep.getNormalizeWRT();
      Normalizator n;
      for (normalization no : normalization.values()) {
         System.out.println("Normalization " + no);
         ep.setDataNormalization(no.toString());
         if (no.equals(normalization.FIXED)) {
            ep.setNormalizeWRT(String.valueOf(costant));
         } else {
            ep.setNormalizeWRT(nonConstantWRT);
         }
         n = Normalizator.buildNormalizator(ep);
         DoubleOutputParser dop = new DoubleOutputParser(ep.getFullData(), ",");
         for (int i = 0; i < dop.numRows(); i++) {
            String[] rawRow = dop.rawRow(i);
            int init = 1; //no benchmark
            for (; init < rawRow.length; init++) {
               double original = Double.parseDouble(rawRow[init]);
               double norm = n.normalize(i, init, original);
               double denorm = n.denormalize(i, init, norm);
               System.out.println(i + ", " + init + " : " + original + " => " + norm + " vs " + denorm);
               if (differMoreThan(original, denorm, 1e-4)) {
                  System.out.println(no + " " + i + ", " + init + " : " + original + " => " + norm + " vs " + denorm);
                  System.exit(9);
               }
            }
         }
      }
   }

   private static boolean differMoreThan(double a, double b, double e) {
      return MathTools.relErr(a, b) > e;
   }

}
