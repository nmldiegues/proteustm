package evaluation.runtimeSimul.configurations;

import java.util.ArrayList;
import java.util.List;
import java.util.SortedSet;
import java.util.TreeSet;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 15/09/14
 */
public class ConfigurationSelectorConfig {

   private ssc mode;     //what does ssc stand for? I cannot remember...
   private long randomSeed;
   private String bootstrapConfigs = "";
   //These are supposed to be for workloads, and should not be relevant to explorable configurations
   //See IntIntRuntimeSimulationLauncher
   private String injectedConfigs = "";
   private final static String sep = ",";
   private int maxExplorations = 10;
   //If true, the max estimated variance will be computed on all the workloads
   private boolean computeMaxVarianceFromAllWorkloads = true;
   private uncertaintyContributeEnum uncertaintyType = uncertaintyContributeEnum.QUADRATIC_DIFFERENCE;
   //True if the contributes to variability have to be normalized wrt mean
   private boolean normalizeVariabilityWRTMean = true;
   //True if --for Variance generator-- the returned variance has to be normalized wrt mean
   private boolean normalizeReturnedVariance = false;
   //True if higher kpi is better
   private boolean isHigherBetter = false;

   private double EIP_threshold = 0.01;
   private boolean doLastJump = false;

      public boolean isDoLastJump() {
         return doLastJump;
      }

      public void setDoLastJump(String doLastJump) {
         this.doLastJump = Boolean.valueOf(doLastJump);
      }

   private stopConditionEnum stopCondition = stopConditionEnum.MAX_EXPLORATIONS;

   public boolean isHigherBetter() {
      return isHigherBetter;
   }

   public void setHigherBetter(String isHigherBetter) {
      this.isHigherBetter = Boolean.valueOf(isHigherBetter);
   }

   public String getInjectedConfigs() {
      return injectedConfigs;
   }


   public double getEIP_threshold() {
      return EIP_threshold;
   }

   public void setEIP_threshold(double EIP_threshold) {
      this.EIP_threshold = EIP_threshold;
   }

   public void setInjectedConfigs(String injectedConfigs) {
      if (injectedConfigs == null) {
         this.injectedConfigs = "";
      } else {
         this.injectedConfigs = injectedConfigs;
      }
   }

   public stopConditionEnum getStopCondition() {
      return stopCondition;
   }

   public void setStopCondition(String stopCondition) {
      this.stopCondition = stopConditionEnum.valueOf(stopCondition);
   }

   public boolean isNormalizeReturnedVariance() {
      return normalizeReturnedVariance;
   }

   public void setNormalizeReturnedVariance(String normalizeReturnedVariance) {
      this.normalizeReturnedVariance = Boolean.valueOf(normalizeReturnedVariance);
   }

   public boolean isNormalizeVariabilityWRTMean() {
      return normalizeVariabilityWRTMean;
   }

   public void setNormalizeVariabilityWRTMean(boolean normalizeVariabilityWRTMean) {
      this.normalizeVariabilityWRTMean = normalizeVariabilityWRTMean;
   }

   public void setNormalizeVarianceWRTMean(String normalizeVarianceWRTMean) {
      this.normalizeVariabilityWRTMean = Boolean.valueOf(normalizeVarianceWRTMean);
   }

   public uncertaintyContributeEnum getUncertaintyType() {
      return uncertaintyType;
   }

   public void setUncertaintyType(String uncertaintyType) {
      this.uncertaintyType = uncertaintyContributeEnum.valueOf(uncertaintyType);
   }

   public boolean isComputeMaxVarianceFromAllWorkloads() {
      return computeMaxVarianceFromAllWorkloads;
   }

   public void setComputeMaxVarianceFromAllWorkloads(boolean computeMaxVarianceFromAllWorkloads) {
      this.computeMaxVarianceFromAllWorkloads = computeMaxVarianceFromAllWorkloads;
   }

   public int getMaxExplorations() {
      return maxExplorations;
   }

   public void setMaxExplorations(int maxExplorations) {
      this.maxExplorations = maxExplorations;
   }

   public SortedSet<Integer> sortedBootstrapConfigs() {
      if (bootstrapConfigs == null || bootstrapConfigs.isEmpty())
         return null;
      String[] configs = this.bootstrapConfigs.split(sep);
      SortedSet<Integer> sortedSet = new TreeSet<>();
      for (String s : configs) {
         sortedSet.add(Integer.parseInt(s));
      }
      return sortedSet;
   }

   public List<Integer> injectedConfigs() {
      if (injectedConfigs == null || injectedConfigs.isEmpty())
         return null;
      String[] configs = this.injectedConfigs.split(sep);
      List<Integer> list = new ArrayList<>();
      for (String s : configs) {
         list.add(Integer.parseInt(s));
      }
      return list;
   }

   public String getBootstrapConfigs() {
      return bootstrapConfigs;
   }

   public void setBootstrapConfigs(String bootstrapConfigs) {
      this.bootstrapConfigs = bootstrapConfigs;
   }

   public long getRandomSeed() {
      return randomSeed;
   }

   public void setRandomSeed(long randomSeed) {
      this.randomSeed = randomSeed;
   }

   public void setMode(String mode) {
      this.mode = ssc.valueOf(mode);
   }

   public boolean isSequential() {
      return is(ssc.SEQUENTIAL);
   }

   public boolean isRandom() {
      return is(ssc.RANDOM);
   }

   public boolean isVariance() {
      return is(ssc.VARIANCE);
   }

   public boolean isGaussian() {
      return is(ssc.GAUSSIAN);
   }

   public boolean isPureGreedy() {
      return is(ssc.PURE_GREEDY);
   }

   private boolean is(ssc isIt) {
      return this.mode == isIt;
   }

   public enum ssc {
      SEQUENTIAL, RANDOM, VARIANCE, NORM_VARIANCE, GAUSSIAN, PURE_GREEDY
   }

   @Override
   public String toString() {
      return "ConfigurationSelectorConfig{" +
            "mode=" + mode +
            ", randomSeed=" + randomSeed +
            ", bootstrapConfigs='" + bootstrapConfigs + '\'' +
            ", injectedConfigs='" + injectedConfigs + '\'' +
            ", maxExplorations=" + maxExplorations +
            ", computeMaxVarianceFromAllWorkloads=" + computeMaxVarianceFromAllWorkloads +
            ", uncertaintyType=" + uncertaintyType +
            ", normalizeVariabilityWRTMean=" + normalizeVariabilityWRTMean +
            ", normalizeReturnedVariance=" + normalizeReturnedVariance +
            ", isHigherBetter=" + isHigherBetter +
            ", EIP_threshold=" + EIP_threshold +
            ", doLastJump=" + doLastJump +
            ", stopCondition=" + stopCondition +
            '}';
   }

   public enum uncertaintyContributeEnum {
      VARIANCE, QUADRATIC_DIFFERENCE
   }

   public enum stopConditionEnum {
      MAX_EXPLORATIONS, CONFIDENCE_CROSS_VALIDATION, LOW_EIP, STRICT_MAPE_CROSS_VALIDATION, AVG_MAPE_CROSS_VALIDATION, EIP_DERIVATIVE
   }

   public boolean isStopMaxExplorations() {
      return stopCondition.equals(stopConditionEnum.MAX_EXPLORATIONS);
   }

   public boolean isStopAtLowEIP() {
      return stopCondition.equals(stopConditionEnum.LOW_EIP);
   }

   public boolean isConfidenceCrossValidationStop() {
      return stopCondition.equals(stopConditionEnum.CONFIDENCE_CROSS_VALIDATION);
   }

   public boolean isStrictMAPEStop() {
      return stopCondition.equals(stopConditionEnum.STRICT_MAPE_CROSS_VALIDATION);
   }

   public boolean isAvgMAPEStop() {
      return stopCondition.equals(stopConditionEnum.AVG_MAPE_CROSS_VALIDATION);
   }

   public boolean isEIPLowDerivativeStop() {
      return stopCondition.equals(stopConditionEnum.EIP_DERIVATIVE);
   }
}

