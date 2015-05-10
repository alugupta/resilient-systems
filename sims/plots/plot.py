import numpy as np
import matplotlib.pyplot as plt

def main():
    filename = "dbiGraph"

    N       = 8

    loadac  = (0.271126760563 ,  0.341455044612  , 0.338863636364  , 0.390  , 0.336305213613  ,0.319  , 0.336, 0.5)
    storeac = (0.00146484375  ,  0.0939597315436 , 0.0737781954887 , 0.0449 , 0.0593011811024 ,0.09   , 0.0642, 0.5)
    loaddc  = (0.184419014085 ,  0.231018359643  , 0.229159090909  , 0.260  , 0.228095582911  ,0.208  , 0.229, 0.5)
    storedc = (0.00146484375  ,  0.0734060402685 , 0.062734962406  , 0.037  , 0.0509350393701 ,0.0663 , 0.059, 0.5)
    print loadac[0]

    ind = np.arange(N) # the x locations for the groups
    width = 0.1       # the width of the bars

    fig, ax = plt.subplots()
    rects1 = ax.bar(ind, loadac, width, color='r')
    rects2 = ax.bar(ind+width, storeac, width, color='y')
    rects3 = ax.bar(ind+2*width, loaddc, width, color='g')
    rects4 = ax.bar(ind+3*width, storedc, width, color='k')

    # add some text for labels, title and axes ticks

    ax.set_ylabel('DBI Ratios')
    ax.set_title('DBI Ratios by MI-Bench Application')
    ax.set_xticks(ind+2*width)
    ax.set_xticklabels(('basicmath', 'jpeg', 'djikstra', 'fft', 'bitcount','blowfish', 'strsearch', 'enc.'))

    ax.legend( (rects1[0], rects2[0], rects3[0], rects4[0]), ('Load AC', 'Store AC', 'Load DC', 'Store DC') )

    plt.savefig("figs/" + filename + ".pdf")
    plt.savefig("figs/" + filename + ".png")
    plt.savefig("figs/" + filename + ".jpg")

if __name__=='__main__':
    main()
