package utilityMatrix.recoEval.datasetLoader.normalization;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 02/10/14
 */
public class ConstantNormalizator {

   private boolean useOnlyRatings = false;
   private boolean invertKpiScore = false;
   private int lastRatingColumn = 224;
   private double min, max;

   private static final int KPI_INDEX = 0;


   public ConstantNormalizator(boolean invertKpiScore, boolean useOnlyRatings, int lastRatingColumn, double min, double max) {
      this.useOnlyRatings = useOnlyRatings;
      this.invertKpiScore = invertKpiScore;
      this.lastRatingColumn = lastRatingColumn;
      this.min = min;
      this.max = max;
   }

   private static double enforce(double score, double min, double max, int user, int item) {
      if (score < min || score > max) {
         throw new RuntimeException("(" + user + ", " + item + ")" + " is not in [" + min + ", " + max + "]");
      }
      return score;
   }

   /**
    * This assumes that I give in input the plain file with
    *
    * @param user
    * @param item
    * @param score
    * @return
    */
   public double normalize(int user, int item, double score) {
      System.out.println("NORMALIZING!");
      final boolean isARating = item < lastRatingColumn;
      final int index = useOnlyRatings ? item : isARating ? KPI_INDEX : item;
      final double norm = featureNormalization.getNormalizationConstantByIndex(index);
      final double normScore = (invertKpiScore && item < lastRatingColumn) ? norm / score : score / norm;
      return enforce(normScore, this.min, this.max, user, item);
   }


   private enum featureNormalization {

      RATING(KPI_INDEX, 1e-4), maxNumRetry(1, 100), avgAbortsPerXact(2, 100), avgAbortsPerWrXact(3, 100), avgAbortsPerRoXact(4, 100),
      updateXactPerc(5, 1), readXactPerc(6, 1), avgWrWSetSize(7, 100), avgWrRSetSize(8, 1e4), avgRoRSetSize(9, 1e4),
      avgNumWritesPerWrXact(10, 1e4), avgUniqueNumWritesPerWrXact(11, 1e4), avgRepeatedNumWritesPerWrXact(12, 1e4),
      avgNumReadBeforeFirstWritePerWrXact(13, 1e4), avgNumRAWReadsPerWrXact(14, 1e4), avgNumNonRAWReadsAfterFirstWritePerWrXact(15, 1e4),
      avgSpearSumReads(16, 1e4), avgNumReadsPerWrXact(17, 1e4), avgNumReadPerRoXact(18, 1e4), avgROXactWCT(19, 1e4),
      avgWrXactWCT(20, 1e4), avgWrCommittedXactWCT(21, 1e4), avgWrAbortedXactWCT(22, 1e5), avgNonTxCodeBlockWCT(23, 1e5), ntcbVTtcbPerc(24, 1);

      private int id;
      private double normalizationConstant;

      featureNormalization(int id, double normalizationConstant) {
         this.id = id;
         this.normalizationConstant = normalizationConstant;
      }

      static double getNormalizationConstantByIndex(int featIndex) {
         for (featureNormalization f : featureNormalization.values()) {
            if (f.id == featIndex) {
               return f.normalizationConstant;
            }
         }
         throw new RuntimeException("Normalization costant not found for index " + featIndex);
      }
   }

   @Override
   public String toString() {
      return "ConstantNormalizator{" +
            "useOnlyRatings=" + useOnlyRatings +
            ", invertKpiScore=" + invertKpiScore +
            ", lastRatingColumn=" + lastRatingColumn +
            ", min=" + min +
            ", max=" + max +
            '}';
   }
}
