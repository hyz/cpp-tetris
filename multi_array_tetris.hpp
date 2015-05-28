#pragma once
#include <time.h>
#include <iostream>
#include <boost/move/move.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/multi_array.hpp"
#include "boost/cstdlib.hpp"
#include "make_array.hpp"

typedef boost::multi_array<char,2> Array2d;
typedef Array2d::array_view<2>::type Array2d_view_t;
using boost::array;
using boost::make_array;

struct Point : array<int,2>
{
	template <typename I>	Point(array<I,2> p) { this->y(p[0]), this->x(p[1]); }
	Point(int y=0, int x=0) { this->y(y), this->x(x); }
	int y() const { return (*this)[0]; }
	int x() const { return (*this)[1]; }
	int y(int v) { return (*this)[0] = v; }
	int x(int v) { return (*this)[1] = v; }
};
struct Size : array<size_t,2>
{
	template <typename I>	Size(array<I,2> p) { this->height(p[0]), this->width(p[1]); }
	Size(size_t a=0, size_t b=0) { this->height(a), this->width(b); }
	size_t height() const { return (*this)[0]; }
	size_t width() const { return (*this)[1]; }
	size_t height(int v) { return (*this)[0] = v; }
	size_t width(int v) { return (*this)[1] = v; }
};

template <typename T>
std::ostream& print2d(std::ostream& out, T const& m)
{
    // return out; //
    for (Point a = make_array(0,0); a[0] != m.size(); ++a[0]) {
        for (a[1] = 0; a[1] != m[0].size()-1; ++a[1])
            out << int( m(a) ) <<" ";
        out << int( m(a) ) <<"\n";
    }
    return out <<"\n";
}

struct V2d : array<int,2>
{
    template <typename V> V2d(V const& a) : array<int,2>(make_array(a[0],a[1])) {}
    friend std::ostream& operator<<(std::ostream& out, V2d const& a) {
        return out <<"<"<< a[0]<<","<<a[1] <<">";
    }
};

template <typename M>
inline array<int,2> get_shape(M const& m)
{
    return make_array( int(m.shape()[0]), int(m.shape()[1]) );
}

template <typename M>
void rotate90_right(M& m)
{
    auto s = get_shape(m);
    BOOST_ASSERT(s[0] == s[1]);
    if (s[0] <= 1)
        return;

    {
        Point p4[4] = {
              make_array(      0, 0      )
            , make_array(      0, s[1]-1 )
            , make_array( s[0]-1, s[1]-1 )
            , make_array( s[0]-1, 0      )
        };
        for (Array2d::index i = 1; i != s[0]; ++i) {
            for (int x=1; x < 4; ++x)
                std::swap(m(p4[0]), m(p4[x]));
            p4[0][1]++; p4[1][0]++; p4[2][1]--; p4[3][0]--;
        }
    }

    if (s[0] == 2)
        return;

    typedef Array2d::index_range range;
    Array2d::array_view<2>::type n = m[boost::indices
            [range(1, s[0]-1)]
            [range(1, s[1]-1)]
        ];
    return rotate90_right(n);
}

inline Point operator-(Point rhs, Point const& lhs)
{
	rhs[0]-=lhs[0]; rhs[1]-=lhs[1];
	return rhs;
    //return Point(rhs[0]-lhs[0], rhs[1]-lhs[1]);
}
template <typename X> void break_p(X) {}

struct Array2dx : Array2d
{
    char dummy_;
    char& at(Point const& a)
    {
        if (a[1] < 0 || a[1] >= int(shape()[1]) || a[0] >= int(shape()[0])) {
            // std::cerr << V2d(a) <<" 1\n"; break_p();
            return (dummy_=1);
        }
        if (a[0] < 0) {
            // std::cerr << V2d(a) <<" 0\n";
            return (dummy_=0);
        }
        return Array2d::operator()(a);
    }
    char  at(Point const& a) const { return const_cast<Array2dx&>(*this).at(a); }
    char  operator()(Point const& a) const { return at(a); }
    char& operator()(Point const& a) { return at(a); }
};

template <typename M, typename N>
bool is_collision(M& va, Point bp, N const& n)
{
    auto s = get_shape(n);
    Point ep = make_array( bp[0]+s[0], bp[1]+s[1] );

    for (auto p=bp; p[0] != ep[0]; ++p[0]) {
        for (p[1] = bp[1]; p[1] != ep[1]; ++p[1]) {
            if (n(p - bp) && va(p)) {
                return 1;
            }
        }
    }
    return 0;
}

template <typename M, typename N>
M& or_assign(M& va, Point bp, N const& n)
{
    auto s = get_shape(n);
    Point ep = make_array( bp[0]+s[0], bp[1]+s[1] );

    for (auto p = bp; p[0] != ep[0]; ++p[0]) {
        for (p[1] = bp[1]; p[1] != ep[1]; ++p[1]) {
            va(p) |= n(p - bp);
        }
    }
    return va;
}

template <typename T,int N>
struct arrayvec : private std::array<T,N>
{
    uint8_t size_;

    arrayvec() { size_=0; clear(); }

    iterator begin() { BOOST_ASSERT(size_>0); return std::array<T,N>::begin(); }
    iterator end() { return begin()+size_; }
	iterator begin() const { return const_cast<arrayvec*>(this)->begin(); }
	iterator end() const { return const_cast<arrayvec*>(this)->end(); }
    int size() const { return size_; }
	bool empty() const { return size()==0; }

    void push_back(T const& x) { BOOST_ASSERT(size_<N); at(size_++) = x; }
    void pop_back() { BOOST_ASSERT(size_>0); size_--; }
    void clear() { size_ = 0; }
};

typedef arrayvec<uint8_t,7> round_result;

struct Tetris_Basic // : Array2d
{
    //std::vector<std::pair<char*,size_t>> const const_mats_;
    Array2dx vmat_;
    Point p_;
    Array2d smat_, pv_;
	boost::posix_time::ptime tb_, td_;
    round_result last_round_result;
    // std::vector<unsigned char> scores_;

    Tetris_Basic() //: const_mats_(mats_init())
    {}

    void new_game(size_t h, size_t w) { reset(h,w); }
    void reset(size_t h, size_t w)
    {
        // scores_.clear();

        vmat_.resize(boost::extents[h][w]);
        for (size_t y=0; y < h; ++y) {
            for (size_t x=0; x < w; ++x)
                vmat_[y][x] = 0;
        }

        pop_preview(0);

        tb_ = boost::posix_time::microsec_clock::local_time();
    }

    bool next_round()
    {
        last_round_result.clear();
        pop_preview(&smat_);

		auto s0 = get_shape(vmat_);
		auto s1 = get_shape(smat_);
        auto tmp = p_ = make_array(-s1[0], (s0[1] - s1[1]) / 2);
        do {
            ++tmp[0];
            if (is_collision(vmat_, tmp, smat_)) {
				or_assign(vmat_, p_, smat_);
                over();
                return 0;
            }
			p_ = tmp;
        } while (p_[0] < 0);
		td_ = boost::posix_time::microsec_clock::local_time();
        return 1;
    }

    bool Move(int di)
    {
        auto tmp = p_;
        if (di == 0) {
            tmp[0]++;
        } else if (di < 0) {
            tmp[1]--;
        } else if (di > 0) {
            tmp[1]++;
        } else {
            rotate();
            return true;
        }

        if (is_collision(vmat_, tmp, smat_)) {
            if (di == 0) {
                or_assign(vmat_, p_, smat_);
                auto sv = get_shape(vmat_);
                auto sa = get_shape(smat_);
				last_round_result.clear();
                collapse(std::min(p_[0]+sa[0], sv[0]-1), std::max(0, p_[0]), last_round_result);
            }
            return false;
        }

        p_ = tmp; //std::cerr << V2d(p_) << "\n";
        if (di == 0) {
			td_ = boost::posix_time::microsec_clock::local_time();
        }
        return true;
    }

    bool rotate()
    {
        auto tmp = smat_;
        rotate90_right(tmp);
        if (is_collision(vmat_, p_, tmp)) {
            return false;
        }
        std::swap(smat_,tmp);
        return true;
    }

private:
    void over()
    {
        std::clog << "over\n";
    }

    void clear(Array2d::index row) {
        for (auto& x : vmat_[row])
            x = 0;
    }
    template <typename R> void move_(Array2d::index src, Array2d::index dst) {
        vmat_[dst] = vmat_[src];
        clear(src);
    }

    void collapse(Array2d::index xlast, Array2d::index xfirst, round_result& res)
    {
        auto const & row = this->vmat_[xlast];

        if (xlast >= xfirst) {
            if (std::find(row.begin(), row.end(), 0) == row.end()) {
                clear(xlast);
                res.push_back( uint8_t(xlast) ); // ++res;
            } else if (!res.empty()) {
                vmat_[xlast+res.size()] = row; clear(xlast);
            }
            if (xlast == xfirst) {
                if (res.empty()) {
                    return;
                }
                // scores_.back() = res; scores_.push_back(0);
            }
        } else if (!res.empty()) {
            // auto pred = [](int x) -> bool { return x!=0; };
            // if (std::find(row.begin(), row.end(), pred) == row.end()) ;
            vmat_[xlast+res.size()] = row; clear(xlast);
        }

        if (xlast > 0) {
            collapse(xlast-1, xfirst, res);
        }
    }

    void pop_preview(Array2d* a)
    {
        if (a) {
            a->resize(boost::extents[pv_.size()][pv_[0].size()]);
            *a = pv_;
            print2d(std::cerr, *a);
        }
        auto p = next_shape_data();
        int x = (int)::sqrt(p.second);
        pv_.resize(boost::extents[x][x]);
        pv_.assign(p.first, p.first + p.second);
        print2d(std::cerr, pv_);
    }

    static std::pair<char*,size_t> next_shape_data()
    {
        static char _2x2[] = {
            1,1,
            1,1
        };
        static char _4x4[] = {
            0, 1, 0, 0,
            0, 1, 0, 0,
            0, 1, 0, 0,
            0, 1, 0, 0
        };
        static char _3x3v[][9] = {
            { 0, 1, 0, //[0]
              1, 1, 1,
              0, 0, 0 },
            { 1, 1, 0, //[1]
              0, 1, 1,
              0, 0, 0 },
            { 0, 1, 1, //[2]
              1, 1, 0,
              0, 0, 0 },
            { 1, 1, 0, //[3]
              0, 1, 0,
              0, 1, 0 },
            { 0, 1, 1, //[4]
              0, 1, 0,
              0, 1, 0 }
        };
        const int N = 2 + sizeof(_3x3v)/(3*3);

        ::srand((int)time(0));
		switch(int x = ::rand() % N) {
		  case 0: return std::make_pair(_2x2, 2*2);
		  case 1: return std::make_pair(_4x4, 4*4);
		  default: return std::make_pair(_3x3v[x-2], 3*3);
        }
    }

    friend std::ostream& operator<<(std::ostream& out, Tetris_Basic const& M)
    {
        auto& m = M.vmat_;
        for (Array2d::index i = 0; i != m.size(); ++i)
        {
            for (Array2d::index j = 0; j != m[0].size()-1; ++j)
            {
                if (i >= M.p_[0] && i < M.p_[0]+M.smat_.size()
                        && (j >= M.p_[1] && j < M.p_[1]+M.smat_[0].size())) {
                    std::clog << int(m[i][j] || M.smat_[i-M.p_[0]][j-M.p_[1]]) <<" ";
                    continue;
                }
				std::clog << int(m[i][j]) << " ";
            }
			std::clog << int(m[i][m[0].size() - 1]) << "\n";
        }
        return out;
    }
};

int main__(int argc, char* const argv[])
{
    Tetris_Basic M;
    M.new_game(20, 10);  //std::cerr << M << "\n";
    while (M.next_round()) {
        M.rotate(); // std::cerr << M << "\n";
        //while (M.Move(-1)) ; std::cerr << M << "\n";
        //while (M.Move( 1)) ; std::cerr << M << "\n";
        while (M.Move(0)) ; std::cerr << M << "\n";
    }
    //std::cerr << M << "\n";
    return 0;
}


