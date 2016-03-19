package utilityMatrix;

import utilityMatrix.recoEval.datasetLoader.normalization.Normalizator;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 17/09/14
 */
public abstract class RecommenderConfig {
   private boolean higherIsBetter = false;
   //True if all the columns are ratings corresponding to kpi in configs (and not workload characterizations)
   private boolean usingOnlyRatings = true;
   //if useOnlyRatings = false, tells us the last rating index
   private int lastRatingColumn;
   //If true, failed ratings won't be substituted by average value
   private boolean allowNullRatings = true;
   //Determines the normalization
   private diffNormalization userItemNormalization = null;
   //Disable prints to file
   private boolean disablePrintToFile = false;

   public boolean isDisablePrintToFile() {
      return disablePrintToFile;
   }

   public void setDisablePrintToFile(String disablePrintToFile) {
      this.disablePrintToFile = Boolean.valueOf(disablePrintToFile);
   }

   //Force the prediction to be non-negative
   private final boolean onlyNonNegative = true;
   private Normalizator.normalization dataNormalization = Normalizator.normalization.NONE;
   private double normalizeWRT = 1;

   public boolean isHigherIsBetter() {
      return higherIsBetter;
   }

   public void setHigherIsBetter(boolean higherIsBetter) {
      this.higherIsBetter = higherIsBetter;
   }

   public void setHigherIsBetter(String higherIsBetter) {
      this.higherIsBetter = Boolean.valueOf(higherIsBetter);
   }

   public double getNormalizeWRT() {
      return normalizeWRT;
   }

   public void setNormalizeWRT(double normalizeWRT) {
      this.normalizeWRT = normalizeWRT;
   }

   public Normalizator.normalization getDataNormalization() {
      return dataNormalization;
   }

   public void setDataNormalization(String dataNormalization) {
      this.dataNormalization = Normalizator.normalization.valueOf(dataNormalization);
   }

   public void setUserItemNormalization(String userItemNormalization) {
      this.userItemNormalization = diffNormalization.valueOf(userItemNormalization);
   }

   public boolean isOnlyNonNegative() {
      return onlyNonNegative;
   }

   public boolean isAllowNullRatings() {
      return allowNullRatings;
   }

   public void setAllowNullRatings(boolean allowNullRatings) {
      this.allowNullRatings = allowNullRatings;
   }

   public boolean isUsingOnlyRatings() {
      return usingOnlyRatings;
   }

   public void setUsingOnlyRatings(boolean usingOnlyRatings) {
      this.usingOnlyRatings = usingOnlyRatings;
   }

   public void setUsingOnlyRatings(String usingOnlyRatings) {
      this.usingOnlyRatings = Boolean.valueOf(usingOnlyRatings);
   }

   public int getLastRatingColumn() {
      return lastRatingColumn;
   }

   public void setLastRatingColumn(int lastRatingColumn) {
      this.lastRatingColumn = lastRatingColumn;
   }

   public boolean isPreprocessMatrix() {
      return this.userItemNormalization != null && this.userItemNormalization != diffNormalization.NONE;
   }

   public boolean isUserNorm() {
      return this.userItemNormalization.wrtUser;
   }

   public boolean isItemNorm() {
      return this.userItemNormalization.wrtItem;
   }

   private boolean isNorm(diffNormalization d) {
      return this.userItemNormalization.equals(d);
   }

   public diffNormalization getUserItemNormalization() {
      return userItemNormalization;
   }

   public enum diffNormalization {

      USER_ONLY(true, false), ITEM_ONLY(false, true), USER_ITEM(true, true), ITEM_USER(true, true), NONE(false, false);

      public boolean wrtUser, wrtItem;

      diffNormalization(boolean wrtUser, boolean wrtItem) {
         this.wrtUser = wrtUser;
         this.wrtItem = wrtItem;
      }
   }

   @Override
   public String toString() {
      return "RecommenderConfig{" +
            "higherIsBetter=" + higherIsBetter +
            ", usingOnlyRatings=" + usingOnlyRatings +
            ", lastRatingColumn=" + lastRatingColumn +
            ", allowNullRatings=" + allowNullRatings +
            ", userItemNormalization=" + userItemNormalization +
            ", disablePrintToFile=" + disablePrintToFile +
            ", onlyNonNegative=" + onlyNonNegative +
            ", dataNormalization=" + dataNormalization +
            ", normalizeWRT=" + normalizeWRT +
            '}';
   }
}