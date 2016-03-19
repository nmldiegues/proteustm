package runtime.oracle;

import evaluation.common.ProteusTMDataSet;
import evaluation.configuration.ConfigurationSelector;
import evaluation.configuration.integer.GaussianIntegerConfigurationSelector;
import evaluation.configuration.integer.IntegerConfiguration;
import evaluation.runtimeSimul.configurations.ConfigurationSelectorConfig;
import evaluation.runtimeSimul.configurations.KNNEnsembleRatingPredictorConfig;
import evaluation.workload.Workload;
import evaluation.workload.integer.IntegerWorkload;
import gnu.trove.map.hash.TIntDoubleHashMap;
import gnu.trove.map.hash.TIntObjectHashMap;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.log4j.PropertyConfigurator;
import runtime.DoubleOutputParser;
import runtime.Pair;
import utilityMatrix.recoEval.datasetLoader.normalization.MinVarianceOnMaxFinder;
import runtime.IntegerToRuntimeConfigFactory;
import utilityMatrix.recoEval.datasetLoader.FixedEntriesCSVLoader;
import utilityMatrix.recoEval.datasetLoader.normalization.Normalizator;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.ExtendedRatingPredictor;
import utilityMatrix.recoEval.evaluationMeasure.recommendation.KNNBaggingEnsembleRatingPredictor;
import utilityMatrix.recoEval.profiles.BasicUserProfile;
import utilityMatrix.recoEval.profiles.BasicUserProfileHolder;
import utilityMatrix.recoEval.profiles.ProfilesHolder;
import utilityMatrix.recoEval.profiles.UserProfile;
import utilityMatrix.recoEval.tools.ExtendedParameters;
import utilityMatrix.recoEval.tools.MathTools;
import xml.DXmlParser;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.util.Set;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 12/02/15
 * <p/>
 * Main daemon to be run to self-tune RecTM at runtime. Most of the methods are borrowed from RuntimeSImulationLauncher
 */

public class ProteusTMOracle {
   protected final static Log log = LogFactory.getLog(ProteusTMOracle.class);
   protected final static boolean info = true;//log.isInfoEnabled();
   protected final static boolean trace = log.isTraceEnabled();
   protected final static String EXPLORE = "EXPLORE";
   protected final static String END_WKLD = "END_WKLD";
   private final static int PORT = 8888;
   private final static String HOST = "";
   private final static String EXT_PARAMS = "ProteusTM/conf/rectm/trainParamsExec.xml";
   private final static String _log_ = "ProteusTM/conf/rectm/log4j.properties";
   private final static boolean debug = log.isDebugEnabled();
   private final static String outFolder = "ProteusTM/rectm";
   private final static int WKLD_OFFSET = 1000;
   private final static String NEW_WKLD = "NEW_WKLD";
   private final static String FEEDBACK = "FEEDBACK";
   private final static String RECTM_CONFIG_SELECTOR = "ProteusTM/conf/rectm/SMBO.xml";
   private final static String SELECTOR_CONFIG = configSelector();
   private final static String _ENS_KNN_CONFIG = "ProteusTM/conf/rectm/predictor/ens_knn.xml";
   protected final ExtendedParameters extendedParameters;
   protected final ConfigurationSelectorConfig configurationSelectorConfig;
   private final Random randomConfigRandom;
   private final List<Pair<Integer, Double>> configKpiList = new ArrayList<>();
   private ProteusTMDataSet trainingSet;
   private ProteusTMDataSet testSet;
   private boolean firstFeedback = true;
   private double normalization = -1;
   private int lastSuggestedConfig = -1;
   private IntegerWorkload currentTrainWorkload = null, currentTestWorkload = null;
   private ConfigurationSelector<IntegerConfiguration> currentConfigSelector;
   private boolean printPredictor = true;

   public ProteusTMOracle(ExtendedParameters extendedParameters, ConfigurationSelectorConfig configurationSelectorConfig) {
      this.extendedParameters = extendedParameters;
      this.configurationSelectorConfig = configurationSelectorConfig;
      this.randomConfigRandom = new Random(configurationSelectorConfig.getRandomSeed());
   }

   private static String configSelector() {
      return RECTM_CONFIG_SELECTOR;
   }

   public static void main(String[] args) {
      PropertyConfigurator.configure(_log_);
      final ExtendedParameters extendedParameters = new DXmlParser<ExtendedParameters>().parse(EXT_PARAMS);
      final ConfigurationSelectorConfig configurationSelectorConfig = new DXmlParser<ConfigurationSelectorConfig>().parse(SELECTOR_CONFIG);
      final ProteusTMOracle proteusTMOracle = new ProteusTMOracle(extendedParameters, configurationSelectorConfig);
         /*Setup ep to use the target normalization, if needed*/
      proteusTMOracle.setupNormalizationIfNeeded();
      proteusTMOracle.setupTrainTest();
         /*Open the socket to accept requests*/
      try {
         ServerSocket serverSocket = new ServerSocket(PORT);
         if (info) {
            log.info("Listening on localhost:" + PORT);
         }
         //So that if client closes connection, you can still accept a new one
         while (true) {
            Socket socket = serverSocket.accept();
            proteusTMOracle.handleClientConnection(socket);
         }
      } catch (IOException ioe) {
         ioe.printStackTrace();
         throw new RuntimeException(ioe);
      }
   }

   protected void resetFirstFeedback() {
      firstFeedback = false;
   }

   protected void resetFirstFeedbackAndSetNormalizationIfNeeded(double d_feedback) {
      if (firstFeedback) {
         normalization = d_feedback;
         firstFeedback = false;
      }
   }

   protected void signalExploredConfiguration(IntegerConfiguration visitedConfig, double kpi) {
      if (info) {
         log.info("Test " + currentTrainWorkload);
         log.info("kpis " + configKpiList);
         log.info("Train " + currentTrainWorkload);
      }
      currentTestWorkload.remove(visitedConfig);
      configKpiList.add(new Pair<>(visitedConfig.getId(), kpi));
      currentTrainWorkload.add(visitedConfig, kpi / normalization);

      if (info) {
         log.info("Test " + currentTrainWorkload);
         log.info("kpis " + configKpiList);
         log.info("Train " + currentTrainWorkload);
      }
   }

   protected void updateConfigurationSelector() {
      ExtendedRatingPredictor extendedRatingPredictor = buildPredictor(trainingSet, currentTestWorkload, testSet);
      extendedRatingPredictor.injectRuntimeFixedNormalizator(normalization);
      this.currentConfigSelector.injectPredictor(extendedRatingPredictor);
   }

   protected int getLastSuggestedConfig() {
      return lastSuggestedConfig;
   }

   protected void setFirstFeedback() {
      firstFeedback = true;
   }

   protected void clearCurrentWkldCfgToKpi() {
      this.configKpiList.clear();
   }

   protected int currentWkldId() {
      return trainingSet.getNumWorkloads() + WKLD_OFFSET + 1;
   }

   protected int numRatings() {
      return trainingSet.getNumConfigsPerWorkload();
   }

   /**
    * Handle a notification of a new incoming workload
    */
   protected void handleNewWorkload(BufferedReader in, PrintWriter out) throws IOException {
      if (info) log.info("New incoming workload: going to initialize");
      //If the previous wkld has not been terminated by a proper END_WKLD, clean the last workload if needed
      removeLastWorkloadIfNeeded();
      //Clear residual from past workload: this covers also the case it has not been completed with an END_WKLD
      clearCurrentWkldCfgToKpi();
      //Set as waiting for first feedback
      setFirstFeedback();

      final int currentWorkloadID = currentWkldId();
      final int numRatings = numRatings();
      testSet = buildInitialTestSet(numRatings);
      currentTestWorkload = dummyIntegerWorkload(currentWorkloadID, numRatings, false);
      currentTrainWorkload = dummyIntegerWorkload(currentWorkloadID, numRatings, true);
      trainingSet.addWorkload(currentTrainWorkload);
      testSet.addWorkload(currentTestWorkload);
      if (trace) log.trace("Current wkld is " + currentWorkloadID);

      this.currentConfigSelector = buildConfigurationSelector(trainingSet, testSet, currentTestWorkload);
      final IntegerConfiguration next = this.currentConfigSelector.next();
      lastSuggestedConfig = next.getId();
      final String configFromInt = IntegerToRuntimeConfigFactory.fromIntToStringCfg(extendedParameters.getFullData(), extendedParameters.getCfgIntToStringParser(), lastSuggestedConfig);
      if (info) {
         log.info("FIRST advised config is " + next.getId() + " corresponding to " + configFromInt.replace("\n", " "));
         log.info("==========");
      }
      out.println(EXPLORE + "\n" + configFromInt);
   }

   protected boolean isLastExploration() {
      return !this.currentConfigSelector.hasNext();
   }

   protected IntegerConfiguration nextConfigurationToExplore() {
      return this.currentConfigSelector.next();
   }

   /**
    * Handle a feedback coming from the client
    *
    * @param in
    * @param out
    * @throws java.io.IOException
    */
   protected void handleFeedback(final BufferedReader in, final PrintWriter out) throws IOException {
      //Grab feedback
      double kpi = 0;
      final String feedback = in.readLine();
      final double d_feedback = Double.parseDouble(feedback);
      if (info) log.info("Feedback = " + d_feedback);
         /*IMPORTANT: when adding a workload, you must start by the max in train, so you should use an offstet!*/
      int feedbackConfig = getLastSuggestedConfig();
      final IntegerConfiguration visitedConfig = new IntegerConfiguration(feedbackConfig);

      kpi = d_feedback;
      resetFirstFeedbackAndSetNormalizationIfNeeded(d_feedback);
      signalExploredConfiguration(visitedConfig, kpi);

      updateConfigurationSelector();
      if (isLastExploration()) {
         final Pair<Integer, Double> opt = bestKnownConfig();
         final int cfg = opt.getFirst();
         final String config = IntegerToRuntimeConfigFactory.fromIntToStringCfg(extendedParameters.getFullData(), extendedParameters.getCfgIntToStringParser(), cfg);
         out.println(END_WKLD + "\n" + config);
         if (info) log.info("No more explorations! Suggesting final config " + cfg + " corresponding to " + config);
         //out.println(opt.getFirst());
         removeLastWorkloadIfNeeded();
      } else {
         if (info) log.info("One more exploration!");
         final IntegerConfiguration next = nextConfigurationToExplore();
         this.lastSuggestedConfig = next.getId();
         final String config = IntegerToRuntimeConfigFactory.fromIntToStringCfg(extendedParameters.getFullData(), extendedParameters.getCfgIntToStringParser(), this.lastSuggestedConfig);
         if (info) {
            log.info("Suggesting " + next + " corresponding to " + config.replace("\n", " "));
            log.info("==========");
         }
         out.println(EXPLORE + "\n" + config);
         //out.println(next.getId());
      }
   }

   /**
    * Handle an incoming connection on the socket workloadSocket
    *
    * @param workloadSocket
    * @throws IOException
    */
   private void handleClientConnection(Socket workloadSocket) throws IOException {
      BufferedReader in = null;
      PrintWriter out = null;
      String inputLine = null;
      try {
         in = new BufferedReader(new InputStreamReader(workloadSocket.getInputStream()));
         out = new PrintWriter(workloadSocket.getOutputStream(), true);
         inputLine = in.readLine();

         if (info) log.info("Received " + inputLine);

         if (inputLine.equals(NEW_WKLD)) {
            handleNewWorkload(in, out);
         } else {
            handleFeedback(in, out);
         }
      } catch (Exception e) {
         e.printStackTrace();
      } finally {
         if (in != null) {
            in.close();
         }
         if (out != null) {
            out.close();
         }
      }
   }

   /**
    * Remove the workload from the training (and test) set, if necessary
    */
   protected void removeLastWorkloadIfNeeded() {
      if (currentTrainWorkload != null) {
         trainingSet.removeWorkload(currentTrainWorkload);
         testSet.removeWorkload(currentTestWorkload);
         currentTrainWorkload = null;
      }
   }

   private boolean isStrictlyBetter(double kpiA, double kpiB) {
      if (extendedParameters.isHigherIsBetter())
         return kpiA > kpiB;
      return kpiA < kpiB;
   }

   protected Pair<Integer, Double> bestKnownConfig() {
      Pair<Integer, Double> opt = null;
      log.info("What is the best config seen so far? Currently I have " + configKpiList.size() + "\n" +
                     configKpiList.toString());
      for (Pair<Integer, Double> pair : configKpiList) {
         if (opt == null) {
            opt = pair;
         } else {
            double obtainedKpi = pair.getSecond();
            if (isStrictlyBetter(obtainedKpi, opt.getSecond())) {
               opt = pair;
            }
         }
      }
      return opt;
   }

   /*
    * Build the initial training set composed of fully profiled random workloads
   */

   private IntegerWorkload dummyIntegerWorkload(int id, int ratings, boolean training) {
      //Training workload is supposed to be empty at beginning
      if (training) {
         return new IntegerWorkload(id, new BasicUserProfile());
      }
      //Testing wl is supposed to have every config...of course here with unknown ratings
      TIntDoubleHashMap tIntDoubleHashMap = new TIntDoubleHashMap();
      for (int i = 1; i <= ratings; i++) {
         tIntDoubleHashMap.put(i, -1);
      }
      return new IntegerWorkload(id, new BasicUserProfile(tIntDoubleHashMap));

   }

   /*
   Setup the training and the test set data structures
    */
   private void setupTrainTest() {
   /*Load the dataset and normalize it*/
      trainingSet = buildInitialTrainingSet();
      testSet = buildInitialTestSet(trainingSet.getNumConfigsPerWorkload());
   }


   /**
    * If normalizing depending on variance, find the proper normalization, set up the parameters and the configuration
    * selector so as to remember to start the exploration with that configuration
    */
   protected void setupNormalizationIfNeeded() {
      if (!extendedParameters.getNormalizeWRT().equals("LOW_VARIANCE")) {
         throw new IllegalArgumentException("Only normalization against low_variance is supported");
      }
      if (extendedParameters.isHigherIsBetter()) {
         MinVarianceOnMaxFinder.setUsingMax();
      }
      //Normalize against lower variance (see paper for details)
      final String normFile = extendedParameters.getFullData();
      final String toNormalizeAgainst;
      final int toNormalizeAgainstIndex;
      final DoubleOutputParser dop;
      try {
         dop = new DoubleOutputParser(extendedParameters.getFullData(), ",");
      } catch (Exception e) {
         throw new RuntimeException(e);
      }
      toNormalizeAgainstIndex = new MinVarianceOnMaxFinder(normFile, extendedParameters.getLastRatingColumn()).lowestVar().getSecond();
      toNormalizeAgainst = dop.headerIfFor(toNormalizeAgainstIndex);
      log.info("Normalizing against " + toNormalizeAgainst);

      //Now set a fixed normalization config, using the one just obtained
      extendedParameters.setDataNormalization("WRT_REF");
      extendedParameters.setNormalizeWRT(toNormalizeAgainst);
      extendedParameters.setReferenceColumnToAdd(toNormalizeAgainst);
      extendedParameters.setNormalizeDependingOnTrainSet("false");
      configurationSelectorConfig.setBootstrapConfigs(toNormalizeAgainstIndex + "");
   }

   private ProteusTMDataSet buildInitialTrainingSet() {
      final int numTrainWKLDS = MathTools.countRowsInFile(new File(extendedParameters.getFullData()), false);
      extendedParameters.setTrainingRow(numTrainWKLDS);
      log.info("Setting " + numTrainWKLDS + " rows in the training set");
      final FixedEntriesCSVLoader trainingLoader = this.trainingSetLoader(extendedParameters);

      try {
         trainingLoader.split();
         final List<Integer> profiledWls = trainingLoader.getPickedRowsForTraining();
         final ProfilesHolder<UserProfile> trainingProfiles = trainingLoader.getUserProfiles();
         if (info) {
            log.info("Total number of workloads " + trainingProfiles.getNbUsers());
            log.info("Total number of configs " + trainingLoader.getNumItemsPerUser());
         }

         if (debug) {
            log.debug("After the split, there are " + trainingProfiles.getNbUsers() + " profiles in the training");
         }
         //Remove the un-profiled workloads from the training, if any
         final int numUsers = trainingProfiles.getNbUsers();
         for (int i = 0; i < numUsers; i++) {
            if (!profiledWls.contains(i)) {
               boolean r = trainingProfiles.removeIfPresent(i) != null;
            }
         }
         final ProteusTMDataSet trainingSet = new ProteusTMDataSet(trainingProfiles, trainingLoader.getNumItemsPerUser());
         if (info) {
            log.info(profiledWls.size() + " Fully profiled training workloads " + profiledWls.toString());
         }
         if (debug) {
            log.debug("Initial training set will have " + trainingSet.getNumWorkloads() + " workloads");
         }
         return trainingSet;
      } catch (IOException e) {
         throw new RuntimeException(e);
      }
   }

   private FixedEntriesCSVLoader trainingSetLoader(ExtendedParameters extendedParameters) {
      final FixedEntriesCSVLoader trainingLoader = new FixedEntriesCSVLoader(extendedParameters.getFullData(), FixedEntriesCSVLoader.TRAIN, extendedParameters.getSeed(), extendedParameters.getTrainingRow(), extendedParameters.getNumCellsPerTestRow(), extendedParameters.getFixedIndexesInTrain());
      final Normalizator normalizator = Normalizator.buildNormalizator(extendedParameters);
      trainingLoader.setNormalizator(normalizator);
      if (info) {
         log.info(normalizator);
      }
      return trainingLoader;
   }

   /**
    * Build an empty training set
    *
    * @param configsPerWl
    * @return
    */
   private ProteusTMDataSet buildInitialTestSet(int configsPerWl) {
      final BasicUserProfileHolder<UserProfile> testProfiles = new BasicUserProfileHolder<UserProfile>(new TIntObjectHashMap<UserProfile>());
      return new ProteusTMDataSet(testProfiles, configsPerWl);
   }

   private ConfigurationSelector<IntegerConfiguration> buildConfigurationSelector(ProteusTMDataSet trainingSet, ProteusTMDataSet testSet, IntegerWorkload workload) {
      ConfigurationSelectorConfig config = this.configurationSelectorConfig;
      log.info(config);
      //Overwriting the random seed with a new one
      config.setRandomSeed(randomConfigRandom.nextInt());
      /*The ConfigurationSelector takes as input also a predictor: in the general case (e.g., typical regression),
      the suggested configuration could be determined without additional feedbacks. In ProteusTM this is not the case,though
      */
      final ExtendedRatingPredictor extendedRatingPredictor = this.buildRatingPredictor(trainingSet.getWorkloadProfiles(), testSet.getWorkloadProfiles(), outFolder, trainingSet.getNumConfigsPerWorkload(), workload);
      if (info) log.info(extendedRatingPredictor);
      return _buildRatingAwareConfigurationSelector(workload, trainingSet, extendedRatingPredictor);
   }

   /**
    * Build a rating predictor. If necessary, properly set up some fields int he config, like the number of items for
    * the mahout-based predictors or the last valid rating index in case we are putting fake columns for workload
    * characterization
    */
   protected ExtendedRatingPredictor buildRatingPredictor(ProfilesHolder<UserProfile> training, ProfilesHolder<UserProfile> testing, String output, int nbItems, Workload currentWl) {
      final ExtendedParameters trainingParams = this.extendedParameters;
      final boolean usingOnlyRatings = trainingParams.isUsingOnlyRatings();
      final int lastValidRatingIndex = trainingParams.getLastRatingColumn();
      final boolean allowNullRatings = trainingParams.isAllowNullRatings();
      final boolean isHigherBtter = trainingParams.isHigherIsBetter();
      if (trainingParams.isEnsembleKNNBag()) {
         final KNNEnsembleRatingPredictorConfig recommenderConfig = new DXmlParser<KNNEnsembleRatingPredictorConfig>().parse(_ENS_KNN_CONFIG);
         recommenderConfig.setNumItemsPerUser(nbItems);
         recommenderConfig.setUsingOnlyRatings(usingOnlyRatings);
         recommenderConfig.setLastRatingColumn(lastValidRatingIndex);
         recommenderConfig.setAllowNullRatings(allowNullRatings);
         recommenderConfig.setHigherIsBetter(isHigherBtter);
         if (printPredictor) {
            printPredictor = false;
            log.info("Ensemble predictor" + recommenderConfig);
         }
         return new KNNBaggingEnsembleRatingPredictor(training, testing, output, trainingParams, recommenderConfig);
      }
      throw new IllegalArgumentException("Rating predictor has not been recognized");
   }

   protected ConfigurationSelector<IntegerConfiguration> _buildRatingAwareConfigurationSelector(IntegerWorkload currentTestWorkload, ProteusTMDataSet trainingSet, ExtendedRatingPredictor predictor) {
      final List<IntegerConfiguration> steadyStateConfigurationsList = new ArrayList<>();
      //These columns correspond to workloads: they don't have to be among the explorable configurations!
      final List<Integer> toBeExcluded = this.configurationSelectorConfig.injectedConfigs();
      final boolean needToExclude = toBeExcluded != null && toBeExcluded.size() > 0;
      //Generate all the possible configurations
      for (int i = 1; i <= trainingSet.getNumConfigsPerWorkload(); i++) {
         if (!(needToExclude && toBeExcluded.contains(i))) {
            steadyStateConfigurationsList.add(new IntegerConfiguration(i));
         }
      }
      final ConfigurationSelectorConfig configurationSelectorConfig = this.configurationSelectorConfig;
      final List<IntegerConfiguration> bootstrapConfigsList = buildBootstrapListAndUpdateSteadyState(configurationSelectorConfig, steadyStateConfigurationsList);
      if (info) {
         log.info("Bootstrap list " + bootstrapConfigsList);
         log.info("Steady-state list " + steadyStateConfigurationsList);
      }
      return __buildRatingAwareConfigurationSelector(bootstrapConfigsList, steadyStateConfigurationsList, configurationSelectorConfig, predictor, currentTestWorkload);
   }

   /**
    * Build the bootstrap list and accordingly update the steady-state one. For now, we are just using a random config
    * as bootstrap TODO: implement normalization w.r.t. fixed config (with fixed config to be profiled first)
    *
    * @param configurationSelectorConfig
    * @param initialList
    * @return
    */
   private List<IntegerConfiguration> buildBootstrapListAndUpdateSteadyState(ConfigurationSelectorConfig configurationSelectorConfig, List<IntegerConfiguration> initialList) {
      final List<IntegerConfiguration> bootstrapConfigsList = new ArrayList<>();
      final Random random = new Random(configurationSelectorConfig.getRandomSeed());
      final Integer[] bootstrapConfigIndices;
      //Take the bootstrap configs: either randomly or from configuration
      if (configurationSelectorConfig.getBootstrapConfigs() == null || configurationSelectorConfig.getBootstrapConfigs().isEmpty()) {
         bootstrapConfigIndices = new Integer[]{1 + random.nextInt(initialList.size() - 1)};    //NB: random is [0,max)
         if (info) {
            log.info("Random initial config: " + bootstrapConfigIndices[0]);
         }
      } else {
         Set<Integer> bootL = configurationSelectorConfig.sortedBootstrapConfigs();
         bootstrapConfigIndices = bootL.toArray(new Integer[bootL.size()]);
         if (info) {
            log.info("Specific initial config(s) " + Arrays.toString(bootstrapConfigIndices));
         }
      }
      //Remove the bootstrap configs from the steady-state list
      List<IntegerConfiguration> toRemoveFromSteadyState = new ArrayList<>();
      for (Integer i : bootstrapConfigIndices) {
         for (IntegerConfiguration ic : initialList)
            if (ic.getId() == i) {
               bootstrapConfigsList.add(ic);
               toRemoveFromSteadyState.add(ic);
            }
      }
      for (IntegerConfiguration ic : toRemoveFromSteadyState) {
         initialList.remove(ic);
         if (trace) {
            log.trace("Removing " + ic + " from steady-state list and adding to bootstrap one");
         }
      }
      return bootstrapConfigsList;
   }

   /**
    * Build the configuration selector //TODO: this can be unified with the Random one, now that they share a common
    * ancestor class
    *
    * @param bootstrapConfigsList
    * @param steadyStateConfigurationsList
    * @param configurationSelectorConfig
    * @param predictor
    * @param currentTestWorkload
    * @return
    */
   private ConfigurationSelector<IntegerConfiguration> __buildRatingAwareConfigurationSelector(List<IntegerConfiguration> bootstrapConfigsList, List<IntegerConfiguration> steadyStateConfigurationsList, ConfigurationSelectorConfig configurationSelectorConfig, ExtendedRatingPredictor predictor, Workload currentTestWorkload) {
      if (configurationSelectorConfig.isGaussian()) {
         return new GaussianIntegerConfigurationSelector(bootstrapConfigsList, steadyStateConfigurationsList, configurationSelectorConfig, predictor, currentTestWorkload);
      }
      throw new IllegalArgumentException("Impossible to generate RatingAwareConfigurationSelector! Check your config");
   }

   private ExtendedRatingPredictor buildPredictor(ProteusTMDataSet trainingSet, Workload testWorkload, ProteusTMDataSet testSet) {
      //Create a profileHolder to serve as test set (only non-profiled configurations for the current workload
      //final ProfilesHolder<UserProfile> currentWorkloadProfileHolder = singleProfileHolder(testWorkload);
      //I am reusing old code, which is not intended to be used dynamically, and prints to file
      //Given I am creating every time a new element, I can force the old code to store and export some info
      //That I would like to be returned here as objects

      final ProfilesHolder<UserProfile> trainingProfiles = trainingSet.getWorkloadProfiles();
      final ProfilesHolder<UserProfile> allTestingProfiles = testSet.getWorkloadProfiles();
      return buildRatingPredictor(trainingProfiles, allTestingProfiles, outFolder, trainingSet.getNumConfigsPerWorkload(), testWorkload);

   }

}
