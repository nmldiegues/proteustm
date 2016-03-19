package utilityMatrix.recoEval.tools;

import utilityMatrix.recoEval.datasetLoader.normalization.Normalizator;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 25/07/14
 */


public class ExtendedParameters extends Parameters {
   //If true, a higher KPI is desirable (maximization problem)
   private boolean higherIsBetter;
   //Num of full rows to insert in the training set
   private int trainingRow;
   //Num of known items for non-fully-profiled workloads
   private int numCellsPerTestRow;
   //Pointer to the csv file with full data sed
   private String fullData;
   //Seed for random number generators
   private long seed;
   //Comma-separated list of indices of items that have to be included in the training set for every user
   private String fixedIndexesInTrain = null; //do not change
   //Recommender to use
   private recmode recmode_;
   //Force the prediction to be non-negative
   private boolean onlyNonNegative = true;
   //True if null ratings are put to negative value. It only works with onlyNonNegative
   private boolean allowNullRatings = true;
   //Id of the run
   private int id;
   private boolean usingOnlyRatings = true;  //do not change
   //last valid rating column index
   private int lastRatingColumn = 224;
   //True if you normalize basing on the current training set
   private boolean normalizeDependingOnTrainSet = true;

   /*Stuff to perform normalization upon split() in test/train*/
   //If true, normalize the dataset when splitting training and testing.
   private String referenceColumnToAdd = "tinystm=5th=MCS_LOCKS";
   private Normalizator.normalization dataNormalization = Normalizator.normalization.NONE;
   //This can be a propert string if we normalize wrt a config, or a number if we normalize wrt a fixed value
   private String normalizeWRT = "tinystm=5th=MCS_LOCKS";

   private boolean realRuntime = true;  //do not change
   //ID of the parser to translate a cfg id to the String parsable by rectm
   private String cfgIntToStringParser = "runtime.LegacyIntegerToRuntimeConfig";


   public String getCfgIntToStringParser() {
      return cfgIntToStringParser;
   }

   public void setCfgIntToStringParser(String cfgIntToStringParser) {
      this.cfgIntToStringParser = cfgIntToStringParser;
   }


   public boolean isRealRuntime() {
      return realRuntime;
   }

   public void setRealRuntime(String realRuntime) {
      this.realRuntime = Boolean.valueOf(realRuntime);
   }

   public boolean isHigherIsBetter() {
      return higherIsBetter;
   }

   public void setHigherIsBetter(boolean higherIsBetter) {
      this.higherIsBetter = higherIsBetter;
   }

   public void setHigherIsBetter(String higherIsBetter) {
      this.higherIsBetter = Boolean.valueOf(higherIsBetter);
   }

   public String getNormalizeWRT() {
      return normalizeWRT;
   }

   public void setNormalizeWRT(String normalizeWRT) {
      this.normalizeWRT = normalizeWRT;
   }

   public Normalizator.normalization getDataNormalization() {
      return dataNormalization;
   }

   public void setDataNormalization(String dataNormalization) {
      this.dataNormalization = Normalizator.normalization.valueOf(dataNormalization);
   }

   public String getReferenceColumnToAdd() {
      return referenceColumnToAdd;
   }

   public void setReferenceColumnToAdd(String referenceColumnToAdd) {
      this.referenceColumnToAdd = referenceColumnToAdd;
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


   public boolean isOnlyNonNegative() {
      return onlyNonNegative;
   }

   public int getId() {
      return id;
   }

   public void setId(int id) {
      this.id = id;
   }

   public ExtendedParameters.recmode getRecmode() {
      return recmode_;
   }

   public void setRecmode(String recmodeS) {
      this.recmode_ = recmode.valueOf(recmodeS);
   }

   public String getFixedIndexesInTrain() {
      return this.fixedIndexesInTrain;
   }

   public static int[] getFixedIndexesInTrainAsIntArray(String indicesString) {
      return splitString(indicesString.split(","));
   }

   public int[] getFixedIndexesInTrainAsIntArray() {
      return splitString(fixedIndexesInTrain.split(","));
   }

   public void setFixedIndexesInTrain(String fixedIndexesInTrain) {
      if (!fixedIndexesInTrain.isEmpty())
         this.fixedIndexesInTrain = fixedIndexesInTrain;

   }

   public long getSeed() {
      return seed;
   }

   public void setSeed(long seed) {
      this.seed = seed;
   }

   public ExtendedParameters(String[] params) {
      super(params);
   }

   public ExtendedParameters() {
   }

   public int getTrainingRow() {
      return trainingRow;
   }

   public void setTrainingRow(int trainingRow) {
      this.trainingRow = trainingRow;
   }

   public String getFullData() {
      return fullData;
   }

   public void setFullData(String fullData) {
      this.fullData = fullData;
   }

   public String getRecmode_() {
      return recmode_.name();
   }

   public int getNumCellsPerTestRow() {
      return numCellsPerTestRow;
   }

   public void setNumCellsPerTestRow(int numCellsPerTestRow) {
      this.numCellsPerTestRow = numCellsPerTestRow;
   }

   public String getBaseDir_() {
      return baseDir_;
   }

   public void setBaseDir_(String baseDir_) {
      this.baseDir_ = baseDir_;
   }

   public String getDatasetPath_() {
      return datasetPath_;
   }

   public void setDatasetPath_(String datasetPath_) {
      this.datasetPath_ = datasetPath_;
   }

   public String getTestingPath_() {
      return testingPath_;
   }

   public void setTestingPath_(String testingPath_) {
      this.testingPath_ = testingPath_;
   }

   public String getOutputPath_() {
      return outputPath_;
   }

   public void setOutputPath_(String outputPath_) {
      this.outputPath_ = outputPath_;
   }

   private static int[] splitString(String[] toSplit) {
      if (toSplit.length == 1 && toSplit[0].equals("")) {
         return null;
      }
      final int[] ret = new int[toSplit.length];
      for (int i = 0; i < toSplit.length; i++) {
         ret[i] = Integer.parseInt(toSplit[i]);
      }
      return ret;
   }

   public boolean isAllowNullRatings() {
      return allowNullRatings;
   }

   public void setAllowNullRatings(boolean allowNullRatings) {
      this.allowNullRatings = allowNullRatings;
   }


   public boolean isEnsembleKNNBag() {
      return this.recmode_.equals(recmode.ENSEMBLE_KNN_BAG);
   }

   public boolean isEnsembleSVDBag() {
      return this.recmode_.equals(recmode.ENSEMBLE_SVD_BAG);
   }


   public boolean isNormalizeDependingOnTrainSet() {
      return normalizeDependingOnTrainSet;
   }

   public void setNormalizeDependingOnTrainSet(String normalizeDependingOnTrainSet) {
      this.normalizeDependingOnTrainSet = Boolean.valueOf(normalizeDependingOnTrainSet);
   }

   @Override
   public String toString() {
      return "ExtendedParameters{" +
            "higherIsBetter=" + higherIsBetter +
            ", trainingRow=" + trainingRow +
            ", numCellsPerTestRow=" + numCellsPerTestRow +
            ", fullData='" + fullData + '\'' +
            ", seed=" + seed +
            ", fixedIndexesInTrain='" + fixedIndexesInTrain + '\'' +
            ", recmode_=" + recmode_ +
            ", onlyNonNegative=" + onlyNonNegative +
            ", allowNullRatings=" + allowNullRatings +
            ", id=" + id +
            ", usingOnlyRatings=" + usingOnlyRatings +
            ", lastRatingColumn=" + lastRatingColumn +
            ", normalizeDependingOnTrainSet=" + normalizeDependingOnTrainSet +
            ", referenceColumnToAdd='" + referenceColumnToAdd + '\'' +
            ", dataNormalization=" + dataNormalization +
            ", normalizeWRT='" + normalizeWRT + '\'' +
            '}';
   }

   public enum recmode {
      ENSEMBLE_KNN_BAG, ENSEMBLE_SVD_BAG
   }

   /**
    * Tells whether we are normalizing w.r.t. a fixed config
    *
    * @return
    */
   public boolean isNormalizingWrtConf() {
      return this.dataNormalization.equals(Normalizator.normalization.WRT_REF);
   }

   public boolean isNormalizingWrtConstant() {
      return dataNormalization.equals(Normalizator.normalization.FIXED);
   }


}
