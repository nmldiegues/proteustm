package evaluation.configuration.integer;

import evaluation.runtimeSimul.configurations.ConfigurationSelectorConfig;
import evaluation.workload.Workload;
import gnu.trove.iterator.TIntIterator;
import umontreal.iro.lecuyer.probdist.NormalDist;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.ExtendedRatingPredictor;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.KNNBaggingEnsembleRatingPredictor;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.MathTools;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 19/09/14
 */


public class GaussianIntegerConfigurationSelector extends VarianceIntegerConfigurationSelector{

   private double lastEIP = -1;
   private double preLastEIP = -1;
   private double previousOpt = -1;

   public GaussianIntegerConfigurationSelector(List<IntegerConfiguration> list, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor predictor, Workload w) {
      super(list, selectorConfiguration, predictor, w);
   }

   public GaussianIntegerConfigurationSelector(List<IntegerConfiguration> bootstrapList, List<IntegerConfiguration> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor predictor, Workload w) {
      super(bootstrapList, steadyStateList, selectorConfiguration, predictor, w);
   }

   @Override
   public String state() {
      return lastEIP + " " + preLastEIP;
   }

   /**
    * @return
    * @throws NoAvailablePredictionsException
    */
   private int maxEIPIndex() throws NoAvailablePredictionsException {
      double[] ret = computeEIP();
      int config = (int) ret[0];
      double maxEIP = ret[1];
      if (maxEIP > lastEIP) {
         preLastEIP = -1;
      } else {
         this.preLastEIP = lastEIP;
      }
      this.lastEIP = maxEIP; //all the candidates have the same maxEIP
      return config;

   }

   private double[] computeEIP() throws NoAvailablePredictionsException {
      final int wlId = currentTestWorkload.getId();
      ratingPredictor.computeRecommendations(wlId);
      final List<UserProfile> userProfiles = currentPredictor().predictedProfiles(wlId);
      //I assume the profiles are homogeneous, so I can take the iterator from the first one in the list
      final TIntIterator configIt = userProfiles.get(0).getItems();
      final double optimalRatingSoFar = ratingPredictor.bestKnownRatingFor(wlId);
      if (trace) {
         log.trace("Optimal rating so far " + optimalRatingSoFar);
      }
      double maxEIP = -1;
      //Configs with max EIP
      List<Integer> candidates = new ArrayList<>();
      while (configIt.hasNext()) {
         int configId = configIt.next();
         //Legacy code...ignore
         if (!ratingPredictor.isActualRating(configId)) {
            continue;
         }
         double measurableMean = computeMeasurableMeanForConfig(configId, userProfiles);
         double variance = computeVarianceForConfig(configId, userProfiles, measurableMean);
         //When we are bagging, it can be that all the learners give the same result, i.e., no variance. In this case we put an arbitrary small variance
         if (variance == 0 && ratingPredictor instanceof KNNBaggingEnsembleRatingPredictor) {
            variance = Double.MIN_VALUE;
         }
         double EIP = EIP(variance, measurableMean, optimalRatingSoFar);
         /*
         EIP should be exactly 0 only if variance is 0 and the mean is < optimal
         However, due to rounding errors with very small values, I guess it is possible to have it 0 here
         even if variance !=0 and mean << current optimal  , i.e., for very small EIPs
         This is why I set the debug level for this
          */
         if (EIP == 0 && debug) {
            log.debug("EIP is 0: MEAN " + measurableMean + " VARIANCE " + variance + " optimalSoFar " + optimalRatingSoFar);
         }

         if (EIP > maxEIP) {
            maxEIP = EIP;
            candidates.clear();
            candidates.add(configId);
         } else if (EIP == maxEIP) {
            candidates.add(configId);
         }
      }

      if (candidates.size() > 1) {    //In case of identical EIP, take one
         Collections.shuffle(candidates, this.random);
      }
      final int maxEIPID = candidates.get(0);
      return new double[]{maxEIPID, maxEIP};
   }

   /**
    * Compute the mean.
    */
   private double computeMeasurableMeanForConfig(int configId, List<UserProfile> userProfiles) throws NoAvailablePredictionsException {
      int validMeasures = 0;
      double sum = 0.0D;
      for (UserProfile u : userProfiles) {
         //sanity check
         if (!u.contains(configId)) {
            throw new RuntimeException("A UserProfile does not contain config " + configId);
         }
         double rating = u.getRating(configId);
         if (rating == _UNKNOWN_PRED)
            throw new NoAvailablePredictionsException("Prediction was impossible for " + u + ", " + configId);
         else {
            validMeasures++;
            sum += rating;
         }
      }
      return sum / (double) validMeasures;
   }

   /**
    * Compute the variance starting from the mean.
    */
   private double computeVarianceForConfig(int configId, List<UserProfile> userProfiles, double measurableMean) throws NoAvailablePredictionsException {
      List<Double> validRatings = new ArrayList<>();
      for (UserProfile u : userProfiles) {
         //sanity check
         if (!u.contains(configId)) {
            throw new RuntimeException("A UserProfile does not contain config " + configId);
         }
         double rating = u.getRating(configId);
         if (rating == _UNKNOWN_PRED)
            throw new NoAvailablePredictionsException("");
         validRatings.add(rating);
      }
      double var = 0;
      for (double d : validRatings) {
         var += (d - measurableMean) * (d - measurableMean);
      }
      //NB: No Bessel correction. If you want, add it. This is indeed a biased variance
      var /= (double) (validRatings.size());
      if (trace) {
         log.trace("C: " + configId + " M: " + measurableMean + " V: " + var + " ==> " + validRatings.toString());
      }
      return var;
   }

   private double EIP(double v2_X, double yhat_X, double y_Xstar) {
      final double v_X = Math.sqrt(v2_X);
      final double mu_X = mu_X(v_X, yhat_X, y_Xstar);

      final NormalDist nd = new NormalDist();
      final double fi_mu_X = nd.density(mu_X);
      final double FI_mu_X = nd.cdf(mu_X);

      final double EIP = v_X * (mu_X * FI_mu_X + fi_mu_X);
      if (EIP < 0 && debug) {
         //Negative EIPs depend from roundings in the computation of FI (which requires computing the erf function)
         //When this happens, it usually means the EIP is negligible
         log.debug("NEGATIVE EIP " + EIP + ": yHat(X) " + yhat_X + " y(X*) " + y_Xstar + " vX " + v_X + " muX " + mu_X + " fi_mu_X" + fi_mu_X + " FI_MU_X " + FI_mu_X);
      }
      return EIP;
   }

   private double mu_X(double v_X, double yhat_X, double y_Xstar) {
      //If it is = -1 it means we are maximizing our target function; else we are minimizing it
      final double ret = (y_Xstar - yhat_X) / v_X;
      return this.selectorConfiguration.isHigherBetter() ? -ret : ret;
   }

   @Override
   protected IntegerConfiguration _next() {
      int maxEIPIndex;
      try {
         maxEIPIndex = maxEIPIndex();
      } catch (NoAvailablePredictionsException n) {
         //...resort to random if you fail
         log.warn("I Could not compute EIP for workload " + currentTestWorkload.getId() + ": returning random config");
         maxEIPIndex = this.random.nextInt(currentList.size());
         preLastEIP = -1;
         lastEIP = -1;
      }
      return new IntegerConfiguration(maxEIPIndex);
   }

   private boolean terminateIfEIPHasLowDerivative(double threshold) {
      double currOptValue = currentPredictor().bestKnownRatingFor(currentTestWorkload.getId());
      //Check we have these two values
      if (lastEIP == -1 || preLastEIP == -1) {
         previousOpt = currOptValue;
         return false;
      }
      //Check we are decreasing
      //1. I could do this over the other previous (pre-pre) or with the current
      if (lastEIP > preLastEIP) {
         previousOpt = currOptValue;
         return false;
      }
      //See if the current EIP, i.e., computed NOW, is promising to be way better

      try {
         final double currEIP = computeEIP()[1];
         log.info("_PE_ A " + currEIP + " " + currOptValue + " " + currEIP / currOptValue);
         if (currEIP / currOptValue > threshold) {
            previousOpt = currOptValue;
            return false;
         }
      } catch (NoAvailablePredictionsException e) {
         //This should not happen: we should always be able at this point to compute the EIP.
         //We could, theoretically, just return false instead of throwing this
         throw new RuntimeException(e);
      }
      //Check whether the previous optimum is close to the current one (within a bound given by threshold)
      //I.e., you had predicted a low EIP and, indeed, we had a low improvement
      double improvementOverPrev = MathTools.relErr(previousOpt, currOptValue);
      if (info) log.info("_PE_ B " + previousOpt + " " + currOptValue + " " + improvementOverPrev);
      previousOpt = currOptValue;
      return improvementOverPrev < threshold;
   }

   @Override
   protected boolean _terminateExploration() {
      return terminateIfEIPHasLowDerivative(selectorConfiguration.getEIP_threshold());
   }




}
