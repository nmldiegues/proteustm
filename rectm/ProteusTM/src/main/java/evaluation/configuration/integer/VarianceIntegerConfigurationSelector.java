package evaluation.configuration.integer;

import evaluation.runtimeSimul.configurations.ConfigurationSelectorConfig;
import evaluation.workload.Workload;
import gnu.trove.iterator.TIntIterator;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.EnsembleExtendedRatingPredictor;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.ExtendedRatingPredictor;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.MathTools;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/09/14
 */
public class VarianceIntegerConfigurationSelector extends AbstractConfigurationSelector<IntegerConfiguration> {


   protected final static double _UNKNOWN_PRED = -1.0D;
   protected final Random random;


   public VarianceIntegerConfigurationSelector(List<IntegerConfiguration> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload workload) {
      super(steadyStateList, selectorConfiguration, ratingPredictor, workload);
      this.random = new Random(selectorConfiguration.getRandomSeed());
   }


   public VarianceIntegerConfigurationSelector(List<IntegerConfiguration> bootstrapList, List<IntegerConfiguration> steadyStateList, ConfigurationSelectorConfig selectorConfiguration, ExtendedRatingPredictor ratingPredictor, Workload workload) {
      super(bootstrapList, steadyStateList, selectorConfiguration, ratingPredictor, workload);
      this.random = new Random(selectorConfiguration.getRandomSeed());
   }

   @Override
   protected IntegerConfiguration _next() {
      final int userId = currentTestWorkload.getId();
      this.ratingPredictor.computeRecommendations(userId);
      final List<UserProfile> predictedProfiles = this.currentPredictor().predictedProfiles(userId);
      int maxVarianceIndex;
      try {
         maxVarianceIndex = maxVarianceConfigIndexForWorkload(userId, predictedProfiles);
      } catch (NoAvailablePredictionsException n) {
         //...resort to random if you fail
         log.warn("I Could not compute variance for workload " + currentTestWorkload.getId() + ": returning random config");
         maxVarianceIndex = this.random.nextInt(currentList.size());
      }
      return new IntegerConfiguration(maxVarianceIndex);
   }


   private int maxVarianceConfigIndexForWorkload(int user, List<UserProfile> predictedProfiles) throws NoAvailablePredictionsException {

      return maxVarianceConfigIndexForWorkloadUsingThisWorkloadVariance(user, predictedProfiles);

   }

   @Override
   protected boolean _terminateExploration() {
      return false; //delegate to parent condition(s), e.g., fixed number of exploration
   }


   /*
    * Find the configuration with the highest variance. If I cannot measure the contribute to the variance for a given
    * prediction, given that it is invalid, I sum the highest variance that I have in input
    *
    * NBBB: the following holds only if I have the variance in input and not the max quadratic difference
    * NB: if the highest input VARIANCE is taken from the current workload, this method actually returns either the config
    * with most failed predictions or the configuration for which we have measured such highest variance input. In fact
    * if a config has 0 predictions, the output variance will be the input
    * If a config has > 1 predictions, the variance will be <= than the input (it cannot be greater, otherwise this config
    * would have been selected to be the config with the highest measurable variance!!)
    * If a config has 1 prediction, it is impossible to compute the mean, and then the average, so it boils down to the
    * case of 0 predictions.
    *
    * If the highest input variance is relevant to all the workloads, if by chance it stems from the current wl, then
    * we go back to the previous case. Otherwise, it is possible that a configuration with X unknown predictions will have
    * a higher variance than a config with Y > X unknown predictions: this may happen if the "known" contribute of the
    * first will be higher than (Y - X)*input
    * EG: max  = 100, 5 predictors
    *    C1 has X=3, known_contribute_1 = 0 (no variance)
    *                                  ==> final variance  = (0 + 3 * max) / 5 = 300/5
    *    C2 has Y=2, known_contribute_2 = 150 (it has still to be that the #of known predictions give a contribute < than
    *    the max possible contribute, i.e., here known_contribute_2 has to be less than (5-3) * max
    *                                 ==> final variance = (150 + 2 * max) / 5 = 350/5
    * @param user
    * @param predictedProfiles
    * @param maxMeasuredVariance
    * @return
    */
   private int configWithHighestVarianceWith(int user, List<UserProfile> predictedProfiles) throws NoAvailablePredictionsException {
      final UserProfile one = predictedProfiles.get(0);
      final TIntIterator it = one.getItems();
      final List<Integer> candidates = new ArrayList<>();
      double highestVariance = -1;
      if (predictedProfiles.size() < 2) {
         log.error("Only " + predictedProfiles.size() + " predicted profiles!!!");
      }
      //Scan all configurations
      while (it.hasNext()) {
         List<Double> validRatings = new ArrayList<>();
         int config = it.next();
         if (!ratingPredictor.isActualRating(config)) {
            continue;
         }
         //Compute the variance in prediction

         for (UserProfile p : predictedProfiles) {
            //sanity check
            if (!p.contains(config)) {
               throw new IllegalArgumentException("User " + user + " does not contain element " + config);
            }
            //Get the rating
            double rating = p.getRating(config);
            //If the rating is valid, then add it to the list directly
            if (rating == _UNKNOWN_PRED)
               throw new NoAvailablePredictionsException("No available prediciton for " + p);
            validRatings.add(rating);
         }

         //Compute the variance for this config
         double predictionVariance = 0;
         double mean = MathTools.average(validRatings.toArray(new Double[validRatings.size()]));
         if (trace) {
            log.trace("Variance on " + validRatings);
         }
         for (double d : validRatings) {
            predictionVariance += (d - mean) * (d - mean);
         }

         //Normalize the RETURNED variance if necessary
         if (selectorConfiguration.isNormalizeReturnedVariance()) {
            predictionVariance /= mean;
         }
         //Check if we have a new winner
         if (trace) {
            log.trace("Current highest variance " + highestVariance + " incumbent " + predictionVariance);
         }

         if (predictionVariance > highestVariance) {
            highestVariance = predictionVariance;
            candidates.clear();
            candidates.add(config);
         }
         //If we have a tie, we save all possible candidates
         else if (predictionVariance == highestVariance) {
            candidates.add(config);
         }

      }
      //Randomly select one of the candidates if there is more than one
      if (candidates.size() > 1) {
         Collections.shuffle(candidates, new Random(selectorConfiguration.getRandomSeed()));
      }
      int hvindex = candidates.get(0);
      if (info)

      {
         log.info("Highest variance is " + hvindex + " with variance " + highestVariance);
      }


      return hvindex;
   }

   private int maxVarianceConfigIndexForWorkloadUsingThisWorkloadVariance(int user, List<UserProfile> predictedProfiles) throws NoAvailablePredictionsException {
      return configWithHighestVarianceWith(user, predictedProfiles);
   }


   protected EnsembleExtendedRatingPredictor currentPredictor() {
      return (EnsembleExtendedRatingPredictor) ratingPredictor;
   }

   protected class NoAvailablePredictionsException extends Exception {
      protected NoAvailablePredictionsException(String message) {
         super(message);
      }
   }

}
