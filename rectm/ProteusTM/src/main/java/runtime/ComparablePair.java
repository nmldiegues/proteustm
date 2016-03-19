package runtime;

/**
 * @author Diego Didona
 * @email didona@gsd.inesc-id.pt
 * @since 20/10/14
 */
public class ComparablePair<A extends Comparable<A>, B extends Comparable<B>> extends Pair<A, B> implements Comparable<Pair<A, B>> {

   public ComparablePair(A first, B second) {
      super(first, second);
   }

   @Override
   public int compareTo(Pair<A, B> o) {
      if (o == null)
         throw new IllegalArgumentException("Impossible to compare w.r.t null");
      if (this == o || this.equals(o))
         return 0;
      int f = this.first.compareTo(o.first);
      if (f != 0)
         return f;
      return this.second.compareTo(o.second);
   }


}
