package utilityMatrix.recoEval.evaluationMeasure.recommendation;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 16/11/14
 */
public class RecNoSuchUserException extends RecommendationException {
   int user;

   public RecNoSuchUserException(int user) {
      this.user = user;
   }
}
