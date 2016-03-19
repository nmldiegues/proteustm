package utilityMatrix.recoEval.profiles;

import gnu.trove.iterator.TDoubleIterator;
import gnu.trove.iterator.TIntDoubleIterator;
import gnu.trove.iterator.TIntIterator;
import gnu.trove.map.TIntDoubleMap;
import gnu.trove.map.hash.TIntDoubleHashMap;
import gnu.trove.set.TIntSet;

import java.util.concurrent.atomic.AtomicReference;

public class BasicUserProfile implements UserProfile {

   private final TIntDoubleMap ratings_;

   private AtomicReference<Double> norm_;

   public BasicUserProfile() {
      this(new TIntDoubleHashMap());
   }

   public BasicUserProfile(TIntDoubleMap ratings) {
      norm_ = new AtomicReference<Double>(null);
      if (ratings != null) {
         ratings_ = ratings;
      } else {
         System.err.println("Warning, creating a BasicUserProfile with null ratings");
         ratings_ = new TIntDoubleHashMap();
      }
   }

   private BasicUserProfile(TIntDoubleMap ratings, AtomicReference<Double> norm) {
      this.norm_ = norm;
      if (ratings != null) {
         ratings_ = ratings;
      } else {
         System.err.println("Warning, creating a BasicUserProfile with null ratings");
         ratings_ = new TIntDoubleHashMap();
      }
   }


   //Just for debug
   public TIntSet keySet() {
      return ratings_.keySet();
   }

   @Override
   public TIntIterator getItems() {
      return ratings_.keySet().iterator();
   }

   @Override
   public boolean contains(int item) {
      return ratings_.containsKey(item);
   }

   @Override
   public double getRating(int item) {
      if (!contains(item)) {
         throw new IllegalArgumentException(this + " does not contain item " + item);
      }
      // if (contains(item)) {
      return ratings_.get(item);
      // } else {
      // throw new
      // RuntimeException("Trying to get the rating of an item that is not here: "
      // + item);
      // }
   }

   @Override
   public int getNbItems() {
      return ratings_.size();
   }

   @Override
   public double getNorm() {
      Double norm = norm_.get();
      if (norm == null) {
         synchronized (this) {
            norm = norm_.get();
            if (norm == null) {
               double squareSum = 0.0;
               for (TDoubleIterator values = ratings_.valueCollection().iterator(); values.hasNext(); ) {
                  double value = values.next();
                  squareSum += value * value;
               }
               norm = Math.sqrt(squareSum);
               norm_.set(norm);
            }
         }
      }
      return norm;
   }


   @Override
   public String toString() {
      return ratings_.toString();
   }

   /**
    * @param item
    * @param rating
    * @return
    */
   @Override
   public boolean add(int item, double rating) {
      if (contains(item))
         return false;
      this.ratings_.put(item, rating);
      //This will force the norm to be recomputed next time
      synchronized (this) {
         norm_ = new AtomicReference<Double>(null);
      }
      return true;
   }

   /**
    * This only works with the HashMap, for now
    *
    * @param item
    * @return
    */
   @Override
   public boolean remove(int item) {
      if (!contains(item))
         return false;
      this.ratings_.remove(item);
      //This will force the norm to be recomputed next time
      synchronized (this) {
         norm_ = new AtomicReference<Double>(null);
      }
      return true;
   }

   public boolean isEmpty() {
      //A shortcut could be evaluate whether the toString is "{}" but I'll go the other way
      TIntIterator it = this.getItems();
      while (it.hasNext()) {
         int n = it.next();
         if (contains(n)) {
            System.out.println("Contains " + n + "! " + getRating(n));
            return false;
         }
      }
      return true;
   }

   public void dump() {
      TIntIterator it = this.getItems();
      while (it.hasNext()) {
         int n = it.next();
         System.out.println(n + " " + this.getRating(n));
      }
   }

   @Override
   public final UserProfile copyUserProfile() {
      synchronized (this) {
         final AtomicReference<Double> norm = new AtomicReference<>(this.norm_.get());
         final TIntDoubleMap map = new TIntDoubleHashMap();
         TIntDoubleIterator it = this.ratings_.iterator();
         while (it.hasNext()) {
            it.advance();
            int i = it.key();
            map.put(i, this.ratings_.get(i));
         }
         return new BasicUserProfile(map, norm);
      }
   }
}
