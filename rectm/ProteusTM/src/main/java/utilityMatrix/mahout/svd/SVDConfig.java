package utilityMatrix.mahout.svd;

import utilityMatrix.mahout.MahoutRecommenderConfig;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 31/08/14
 * <p/>
 * I will use a single configuration file for every Factorizers, instead of having one for each
 */
public class SVDConfig extends MahoutRecommenderConfig {
   public final static double _MHT_DEF_LEARN = 0.01D;
   public final static double _MHT_DEF_OVF = 0.1D;
   public final static double _MHT_DEF_NOISE = 0.01D;
   public final static double _MHT_DEF_DECAY = 1.0D;
   public final static int _DEF_IT = 500;
   public final static double _DEF_LAM = 0.065;

   //NB: these default values should actually be the default in Mahout implementation
   private double learningRate = _MHT_DEF_LEARN;
   //I think this is the regularization parameter
   private double preventOverfitting = _MHT_DEF_OVF;
   private double randomNoise = _MHT_DEF_NOISE;
   private double learningRateDecay = _MHT_DEF_DECAY;
   private int numIterations = _DEF_IT;

   private double lambda = _DEF_LAM;

   private factorization factorizer;
   //This is to bypass the config default values and use mahout's ones
   private boolean useDefaultValues;

   private latentFeaturesMode latentFeaturesModality = latentFeaturesMode.PERCENTAGE;
   private double latentFeatures = 0.25D;

   private String tempFile = "UtilityMatrix/conf/temp.data";

   private final boolean useImplicitFeedback = false;
   private final double alpha = 1;
   private int numThreads = 4;


   public int getNumThreads() {
      return numThreads;
   }

   public double getLambda() {
      return lambda;
   }

   public void setLambda(double lambda) {
      this.lambda = lambda;
   }

   public String getTempFile() {
      return tempFile;
   }

   public void setTempFile(String tempFile) {
      this.tempFile = tempFile;
   }

   public boolean fixedNumLatentFeatures() {
      return this.latentFeaturesModality.equals(latentFeaturesMode.FIXED);
   }

   public void setLatentFeaturesModality(String latentFeaturesModality) {
      this.latentFeaturesModality = latentFeaturesMode.valueOf(latentFeaturesModality);
   }

   public double getLatentFeatures() {
      return latentFeatures;
   }

   public void setLatentFeatures(double latentFeatures) {
      this.latentFeatures = latentFeatures;
   }


   public void setFactorizer(String factorizer) {
      this.factorizer = factorization.valueOf(factorizer);
   }

   public boolean isUseDefaultValues() {
      return useDefaultValues;
   }

   public void setUseDefaultValues(String useDefaultValues) {
      this.useDefaultValues = Boolean.valueOf(useDefaultValues);
   }

   public double getLearningRate() {
      return learningRate;
   }

   public void setLearningRate(double learningRate) {
      this.learningRate = learningRate;
   }

   public double getPreventOverfitting() {
      return preventOverfitting;
   }

   public void setPreventOverfitting(double preventOverfitting) {
      this.preventOverfitting = preventOverfitting;
   }

   public double getRandomNoise() {
      return randomNoise;
   }

   public void setRandomNoise(double randomNoise) {
      this.randomNoise = randomNoise;
   }

   public int getNumIterations() {
      return numIterations;
   }

   public void setNumIterations(int numIterations) {
      this.numIterations = numIterations;
   }

   public double getLearningRateDecay() {
      return learningRateDecay;
   }

   public void setLearningRateDecay(double learningRateDecay) {
      this.learningRateDecay = learningRateDecay;
   }

   public boolean isSGD() {
      return this.factorizer.equals(factorization.SGD);
   }

   public boolean isSGDPlusPlus() {
      return this.factorizer.equals(factorization.SGDPlusPlus);
   }

   public boolean isALS() {
      return this.factorizer.equals(factorization.ALSWRF);
   }

   private enum factorization {
      SGD, SGDPlusPlus, ALSWRF
   }

   private enum latentFeaturesMode {
      FIXED, PERCENTAGE
   }

   public factorization getFactorizer() {
      return factorizer;
   }

   public latentFeaturesMode getLatentFeaturesModality() {
      return latentFeaturesModality;
   }

   @Override
   public String toString() {
      return super.toString() + "SVDConfig{" +
            "learningRate=" + learningRate +
            ", preventOverfitting=" + preventOverfitting +
            ", randomNoise=" + randomNoise +
            ", learningRateDecay=" + learningRateDecay +
            ", numIterations=" + numIterations +
            ", factorizer='" + factorizer + '\'' +
            ", useDefaultValues=" + useDefaultValues +
            ", latentFeaturesModality=" + latentFeaturesModality +
            ", latentFeatures=" + latentFeatures +
            '}';
   }
}
